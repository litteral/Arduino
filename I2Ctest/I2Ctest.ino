

#include <Wire.h>
#include "TSL2561.h"
#include <Adafruit_BMP085.h>

#include <RTClib.h>
RTC_DS1307 RTC;
Adafruit_BMP085 bmp;
// Example for demonstrating the TSL2561 library - public domain!

// connect SCL to analog 5
// connect SDA to analog 4
// connect VDD to 3.3V DC
// connect GROUND to common ground
// ADDR can be connected to ground, or vdd or left floating to change the i2c address

// The address will be different depending on whether you let
// the ADDR pin float (addr 0x39), or tie it to ground or vcc. In those cases
// use TSL2561_ADDR_LOW (0x29) or TSL2561_ADDR_HIGH (0x49) respectively
TSL2561 tsl(TSL2561_ADDR_FLOAT); 

void setup() {
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  bmp.begin();  
  if (tsl.begin()) {
    Serial.println("Found sensor");
  } 
  else {
    Serial.println("No sensor?");
    while (1);
  }
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)

  // Now we're ready to get readings!
}

void loop() {
  // Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 13-402 milliseconds! Uncomment whichever of the following you want to read
  //uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);     
  //uint16_t x = tsl.getLuminosity(TSL2561_FULLSPECTRUM);
  //uint16_t x = tsl.getLuminosity(TSL2561_INFRARED);

  //Serial.println(x, DEC);

  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparions you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print("IR: "); 
  Serial.println(ir);//   Serial.print("\t\t");
  Serial.print("Full: "); 
  Serial.println(full);//   Serial.print("\t");
  Serial.print("Visible: "); 
  Serial.println(full - ir);//   Serial.print("\t");

  Serial.print("Lux: "); 
  Serial.println(tsl.calculateLux(full, ir));
  DateTime now = RTC.now();//Read the RTC
  const uint8_t h = now.hour();
  const uint8_t hr_12 = h%12;
  //Serial.print("connection failed at : ");
  //Serial.print();
  Serial.print("Time        : ");
  if(hr_12 < 10){                // Zero padding if value less than 10 ie."09" instead of "9"
    Serial.print(" ");
    Serial.print((h > 12) ? h - 12 : ((h == 0) ? 12 : h), DEC); // Conversion to AM/PM  
  }
  else{
    Serial.print((h > 12) ? h - 12 : ((h == 0) ? 12 : h), DEC); // Conversion to AM/PM
  }
  Serial.print(':');
  if(now.minute() < 10){         // Zero padding if value less than 10 ie."09" instead of "9"
    Serial.print("0");
    Serial.print(now.minute(), DEC);
  }
  else{
    Serial.print(now.minute(), DEC);
  }
  if(h < 12){                  // Adding the AM/PM sufffix
    Serial.print(" AM");
  }
  else{
    Serial.print(" PM");
  }
  Serial.println();
  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.print(" *F");
  Serial.println();
  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure()* 0.0002953) * 0.0002953;
  Serial.print(" Hg");
  Serial.println();
  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  Serial.print("Altitude = ");
  Serial.print(bmp.readAltitude(102500));
  Serial.print(" M");
  Serial.println();
  float Batt = analogRead (A0) * 1.92 * 5.0 / 1023.0;
  Serial.print("Solar ");
  Serial.print(Batt);
  Serial.println();
  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1025 millibars
  // that is equal to 102500 Pascals.
  Serial.print("Altitude = ");
  Serial.print(bmp.readAltitude(102500)/3.3);
  Serial.println(" Feet OMSL");
  Serial.println();
  delay(3000);
}

