#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
//#include <stdlib.h>
#include <stddef.h>

#include <Arduino.h>
//#include <Wire.h>

#include "twi.h"

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


void twi_hello_world() {
   struct twi_receive_t response;

   twi_init();

   while(1) {
      // Read config register to see if data is ready
      if (twi_command(TC74_ADDRESS, TC74_RWCR_COMMAND) == MT_DATA_ACK) {
         response = twi_receive_byte_nak(TC74_ADDRESS);
         if (response.status == MR_DATA_RCVD_NAK  // if we got status,
             && (response.data & TC74_DATA_READY) // and data is ready,
             && twi_command(TC74_ADDRESS, TC74_RTR_COMMAND) == MT_DATA_ACK) // then read temperature register
         {
            response = twi_receive_byte_nak(TC74_ADDRESS);
            if (response.status == MR_DATA_RCVD_NAK) {
               Serial.print(" TEMP:");
               Serial.println(response.data);
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