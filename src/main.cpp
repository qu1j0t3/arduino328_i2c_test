#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
//#include <stdlib.h>
#include <stddef.h>

#include <Arduino.h>
#include <Wire.h>

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


int main() {
   //uint8_t tick = 0;

   // define outputs
   //DDRB = (1 << LED_PIN) | (1 << RX_PIN);

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

   Wire.begin();
   Wire.setTimeout(100);
   Wire.setWireTimeout(50000, true);

   while(1) {
      _delay_ms(200);
      Serial.println("Test");
   Serial.flush();

      Wire.beginTransmission(TC74_ADDRESS);
      Serial.println('A');
   Serial.flush();
      Wire.write(0x01);
      Serial.println('B');
   Serial.flush();
      uint8_t err = Wire.endTransmission(0);
      Serial.println(err);
   Serial.flush();

      uint8_t err2 = Wire.requestFrom(TC74_ADDRESS, 1, true);
      Serial.println(err2);
   Serial.flush();
      uint8_t status = Wire.read();
      Serial.println(status);
      Serial.println("---");
   Serial.flush();

   }

}