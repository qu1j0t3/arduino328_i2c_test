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

void twi_hello_world() {
  TWSR = TWI_PRESCALER_4;
  TWBR = 18; // Aim at 100kHz with Prescaler /4

  TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);

  PIND |= 1 << PORTD_TRIGGER;

  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send START condition


  while(!(TWCR & (1<<TWINT))) // wait for end of tx
    ;

  _delay_us(100);

  PIND |= 1 << PORTD_TRIGGER;

  if ((TWSR & 0xf8) != 0x08) {
    Serial.println("status:");
    Serial.println(TWSR & 0xf8);
    Serial.println("Error@1");
  } else {
    Serial.println("OK@1");
    // send address
    TWDR = (0x55 << 1)|1; // dummy address
    TWCR = (1<<TWINT)|(1<<TWEN);

    while(!(TWCR & (1<<TWINT))) // wait for end of tx
      ;

    if ((TWSR & 0xf8) == 0x18) {
      Serial.println("ACK@2");
    } else {
      Serial.println("status:");
      Serial.println(TWSR & 0xf8);
      Serial.println("Error@2");
    }
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
        }

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
                  if ((tick % 8) == 0) {
                     Serial.println(temp);
                  } else {
                     Serial.print(temp);
                  }
                  Serial.flush();
            }
        }

        _delay_ms(1000);
    }

}