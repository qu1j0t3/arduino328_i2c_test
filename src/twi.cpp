#include <avr/io.h>
#include <util/delay.h>

#include <stdint.h>

#include "twi.h"

enum{  // will be used to set TWEA
   I2C_NAK,
   I2C_ACK
};

enum {
  I2C_WRITE,
  I2C_READ
}; // data direction

enum {
  TWI_PRESCALER_1 = 0,
  TWI_PRESCALER_4,
  TWI_PRESCALER_16,
  TWI_PRESCALER_64
};

// Note the following (spec Table 11):
enum {
   T_SETUP_STOP_US = 4, // Set-up time for STOP condition, min 4µs
   T_SETUP_REP_START_US = 5, // Set-up time for repeated START condition, min 4.7µs
   T_HOLD_REP_START_US = 4, // Hold time for repeated START condition, min 4µs
   T_BUS_FREE_US = 5,    // Bus free time between a STOP and START condition, min 4.7µs
   SCL_CLOCK = 100000L /* I2C clock in Hz */
};

void twi_init() {
   TWSR = TWI_PRESCALER_1;
   TWBR = ((F_CPU/SCL_CLOCK)-16)/2;  /* must be > 10 for stable operation */
}

uint8_t twi_wait() {
   // When the TWI has finished an operation and expects
   // application response, the TWINT Flag is set
   while(!(TWCR & (1<<TWINT))) // wait for end of tx
     ;
   return TWSR & 0xf8;
}

uint8_t twi_send(uint8_t b) {
   // When the TWINT Flag is set, the user must update all TWI
   // Registers with the value relevant for the next TWI bus cycle.

   TWDR = b;

    // After all TWI Register updates and other pending application
    // software tasks have been completed, TWCR is written.
    // When writing TWCR, the TWINT bit should be set.
    // Writing a one to TWINT clears the flag. The TWI will then commence
    // executing whatever operation was specified by the TWCR setting.

   TWCR = (1<<TWINT)|(1<<TWEN);

   return twi_wait();
}

uint8_t twi_start(uint8_t addr, bool is_read) {
   TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA);
   uint8_t st = twi_wait();
   return st == MT_START_SENT ? twi_send((addr << 1) | is_read) : st;
}

void twi_stop() {
   TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
   _delay_us(T_BUS_FREE_US);
}

struct twi_receive_t twi_receive(bool ack) {
   TWCR = (1<<TWEN)|(1<<TWINT)|(ack<<TWEA);
   uint8_t st = twi_wait();
   return { .data = TWDR, .status = st };
}

uint8_t twi_command(uint8_t addr, uint8_t cmd) {
   uint8_t st = twi_start(addr, I2C_WRITE);
   if (st == MT_SLA_ACK) {
      st = twi_send(cmd);
   }
   twi_stop();
   return st;
}

struct twi_receive_t twi_receive_byte_nak(uint8_t addr) {
   struct twi_receive_t resp;
   resp.status = twi_start(addr, I2C_READ);
   if (resp.status == MR_SLA_ACK) {
      resp = twi_receive(I2C_NAK);
   }
   twi_stop();
   return resp;
}
