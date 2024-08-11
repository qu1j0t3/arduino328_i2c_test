#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
//#include <stdlib.h>
#include <stddef.h>

#include <Arduino.h>
//#include <Wire.h>

extern "C" {
  #include "fleury_i2cmaster/i2cmaster.h"
};

/*
 ┏━━━━━━━━━━━┓
 ┃ 2   4   6 ┃  ISP header socket
 ┃ 1   3   5 ┃  on Digispark carrier protoboard
 ┗━━━     ━━━┛
 See: https://www.pololu.com/docs/0J36/all
 1:       = MISO        = ATtiny85 pin 1
 2: Red   = Target VDD 5V
 3: Green = SCK         = ATtiny85 pin 2
 4: Black = MOSI/SDA    = ATtiny85 pin 0
 5:       = _RST        = ATtiny85 pin 5
 6: White = GND
 */

enum {
   SCD41_ADDRESS = 0x62,
   // Note TC74 part # : TC74XX-YYZAA
   //    where XX is the address code, e.g. A0 = 0b1001000
   TC74_ADDRESS = 0b1001000
};// i2c_addresses;

enum{
   MT_START_SENT = 0x08, // see datasheet Table 22-2
   MT_REP_START_SENT = 0x10,
   MT_SLA_ACK = 0x18,  // after SLA+W
   MT_SLA_NAK = 0x20,  // after SLA+W
   MT_DATA_ACK = 0x28, // after data byte
   MT_DATA_NAK = 0x30, // after data byte
   MT_ARB_LOST = 0x38, // after SLA+W
   MR_SLA_ACK = 0x40,  // after SLA+R
   MR_SLA_NAK = 0x48,  // after SLA+R
   MR_DATA_RCVD_ACK = 0x50,
   MR_DATA_RCVD_NAK = 0x58
};
enum{  // will be used to set TWEA
   I2C_NAK,
   I2C_ACK
};

enum {
  TC74_RTR_COMMAND = 0,
  TC74_RWCR_COMMAND
};

enum {
   TC74_NORMAL_MODE = 0,
   TC74_STANDBY_MODE = 1<<7, // bit written to CR
   TC74_DATA_READY = 1<<6 // bit read from CR
};

enum {
  PORTD_TRIGGER = 2
};// io_map

enum {
  TWI_PRESCALER_1 = 0,
  TWI_PRESCALER_4,
  TWI_PRESCALER_16,
  TWI_PRESCALER_64
};


uint8_t twi_wait() {
   // When the TWI has finished an operation and expects
   // application response, the TWINT Flag is set
   while(!(TWCR & (1<<TWINT))) // wait for end of tx
     ;
   return TWSR & 0xf8;
}

uint8_t twi_send(uint8_t b) {
   // When the TWINT Flag is set, the user must update all TWI Registers with the value relevant for the next TWI bus cycle.

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
   _delay_us(4); // Min for TC74
}

struct twi_receive_t {
   uint8_t data;
   uint8_t status;
};

struct twi_receive_t twi_receive(bool ack) {
   TWCR = (1<<TWEN)|(1<<TWINT)|(ack<<TWEA);
   uint8_t st = twi_wait();
   return { .data = TWDR, .status = st };
}

void twi_hello_world() {
   struct twi_receive_t response;

   // initialisation copied from Fleury
   enum{ SCL_CLOCK = 100000L };      /* I2C clock in Hz */
   TWSR = 0;                         /* no prescaler */
   TWBR = ((F_CPU/SCL_CLOCK)-16)/2;  /* must be > 10 for stable operation */

   while(1) {
      if (twi_start(TC74_ADDRESS, I2C_WRITE) == MT_SLA_ACK
          && twi_send(TC74_RWCR_COMMAND) == MT_DATA_ACK) {
         twi_stop();

         if (twi_start(TC74_ADDRESS, I2C_READ) == MR_SLA_ACK) {
            response = twi_receive(I2C_NAK);
            twi_stop();

            if (response.status == MR_DATA_RCVD_NAK) {
               if ((response.data & TC74_DATA_READY)
                  && twi_start(TC74_ADDRESS, I2C_WRITE) == MT_SLA_ACK
                  && twi_send(TC74_RTR_COMMAND) == MT_DATA_ACK)
               {
                  twi_stop();

                  if (twi_start(TC74_ADDRESS, I2C_READ) == MR_SLA_ACK) {
                     response = twi_receive(I2C_ACK);
                     twi_stop();

                     if (response.status == MR_DATA_RCVD_ACK) {
                        Serial.print(" TEMP:");
                        Serial.println(response.data);
                     }
                  }
               }
            }
         }
      }

      Serial.flush();
      _delay_ms(1000);
   }
}

int main() {
   uint8_t cr;
   uint8_t temp;

   // define outputs
   //DDRD = (1 << PORTD_TRIGGER);
   //PORTD = 0;

Serial.begin(9600);
      _delay_ms(200);

   Serial.println("Ready.");
   Serial.flush();

   twi_hello_world();
   Serial.flush();
   return 0;

    // Pinout for I2C ...
    // Duemilanove schematic for the programming connector:
    // ICSP PIN       IO PIN
    //    3           PB5 (SCK)   IC pin 19    J3 pin 13
    //    4           PB3 (MOSI)  IC pin 17    J3 pin 11

    // 328 datasheet says that its Two Wire Interface uses:
    //   SCL on Port C, Bit 5 (IC pin 28, J2 pin 6)
    //   SDA on Port C, Bit 4 (IC pin 27, J2 pin 5)
    // J2 is the "Analog" header that is next to the power header,
    // & these should be the pins numbered 4, 5 on the silkscreen.

    i2c_init();

   uint8_t ret = i2c_start((TC74_ADDRESS << 1) | I2C_WRITE);       // set device address and write mode

   if ( ret ) { // failed to issue start condition, possibly no device found
      i2c_stop();
      Serial.println("InitError1");
   } else {// issuing start condition ok, device accessible
      i2c_write(TC74_RWCR_COMMAND);
      i2c_write(TC74_NORMAL_MODE); // turn off Standby mode
      i2c_stop();
   }

    for(uint8_t tick = 0; ; ++tick) {
        uint8_t ret = i2c_start((TC74_ADDRESS << 1) | I2C_WRITE);       // set device address and write mode

        if ( ret ) { // failed to issue start condition, possibly no device found
            i2c_stop();
            Serial.println("Error1");
        } else {// issuing start condition ok, device accessible
            i2c_write(TC74_RWCR_COMMAND);
            i2c_stop();

            i2c_start((TC74_ADDRESS << 1) | I2C_READ);     // set device address and write mode
            cr = i2c_readNak();                    // read one byte
            i2c_stop();

        if (cr & TC74_DATA_READY) {
            uint8_t ret = i2c_start((TC74_ADDRESS << 1) | I2C_WRITE);       // set device address and write mode

            if ( ret ) { // failed to issue start condition, possibly no device found
                  i2c_stop();
                  Serial.println("Error2");
            } else {// issuing start condition ok, device accessible
                  i2c_write(TC74_RTR_COMMAND);
                  i2c_stop();

                  i2c_start((TC74_ADDRESS << 1) | I2C_READ);     // set device address and write mode
                  temp = i2c_readNak();                    // read one byte
                  i2c_stop();

                  Serial.print(" TEMP:");
                  if ((tick % 8) == 7) {
                     Serial.println(temp);
                  } else {
                     Serial.print(temp);
                  }
                  Serial.flush();
            }
         }

         }

        _delay_ms(1000);
    }

}