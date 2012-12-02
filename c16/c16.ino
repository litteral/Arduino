
//Binary sketch size: 28,570 bytes (of a 32,256 byte maximum)
/*
 *  Anemometer code modified for use with an NRG40c Anemometer- 
 *-converted to use a single hall effect sensor
 * ORIGINAL Authors: M.A. de Pablo & C. de Pablo S., 2010
*/
#include <Wire.h>
#include <TSL2561.h>
#include "RTClib.h"
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_BMP085.h>
#include "TSL2561.h"
/*
AM2302 (wired DHT22) temperature-humidity sensor
http://www.adafruit.com/products/393
-------------------------------------------------------------------------------------
 Connection for the ADAFRUIT TSL2561 digital luminosity / lux / light sensor
 https://www.adafruit.com/products/439 

 connect SCL to analog 5
 connect SDA to analog 4
 connect VDD to 3.3V DC
 connect GROUND to common ground
 ADDR can be connected to ground, or vdd or left floating to change the i2c address

 The address will be different depending on whether you let
 the ADDR pin float (addr 0x39), or tie it to ground or vcc. In those cases
 use TSL2561_ADDR_LOW (0x29) or TSL2561_ADDR_HIGH (0x49) respectively
-------------------------------------------------------------------------------------
Connection for the ADAFRUIT BMP085 Barometric Pressure/Temperature/Altitude Sensor- 5V ready!
https://www.adafruit.com/products/391
 Connect VCC of the BMP085 sensor to 3.3V (NOT 5.0V!)
 Connect GND to Ground
 Connect SCL to i2c clock - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 5
 Connect SDA to i2c data - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 4
 EOC is not used, it signifies an end of conversion
 XCLR is a reset pin, also not used here
-------------------------------------------------------------------------------------
 Connections for the ADAFRUIT DS1307 Real Time Clock breakout board kit
 https://www.adafruit.com/products/264
// Date and time functions using a DS1307 RTC connected via I2C and Wire lib.
--------------IMPORTANT NOTE ON THE ADAFRUIT DS1307---------------------------------- 
DoNot connect the two resistors if making this BOB, If you have already made the RTC DS1307 BOB
DISCONNECT thge two resistors Desolder or CLIP them!!!
-------------------------------------------------------------------------------------
Also: some nice code to display 12 hr....   :-)
    DateTime now = RTC.now();//Read the RTC
    const uint8_t h = now.hour();
    const uint8_t hr_12 = h%12;
    Serial.print("connection failed at : ");
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
-------------------------------------------------------------------------------------    
*/
//The Beginning
TSL2561 tsl(TSL2561_ADDR_FLOAT); 
#define ANEMOMETER 2 
#define DHTTYPE DHT22 
#define DHTPIN (A2) 

//COSM info
#define APIKEY "d5acRBGU12Y0jGMK5rGi_65vri2SAKxTWDFqNkg5c3dIND0g" // your cosm api key
#define FEEDID 72737   // feed ID
#define USERAGENT "Cosm Arduino Example (72737 )" // user agent is the project name
//DHT info
DHT dht(DHTPIN, DHTTYPE);
//BARO/TEMP info
Adafruit_BMP085 bmp;

// Constants definitions for Anemometer
const float pi = 3.14159265;  // pi number
int period = 3000;           // Measurement period (miliseconds)
int delaytime = 3000;        // Time between samples (miliseconds)
int radio = 70; //NRG40c (70) Radius from vertical anemometer axis to a cup center (mm)
char* winds[] = {
  "Calm", "Light air", "Light breeze", "Gentle breeze", "Moderate breeze", "Fresh breeze", "Strong breeze", "Moderate gale", "Fresh gale", "Strong gale", "Storm", "Violent storm", "AHHHHHHHH!   Hurricane"};

// Variable definitions for Anemometer

unsigned int Sample = 0;       // Sample number
unsigned int counter = 0;      // B/W counter for sensor 
unsigned int RPM = 0;          // Revolutions per minute
float speedwind = 0;           // Wind speed (m/s)
unsigned short windforce = 0;  // Beaufort Wind Force Scale

// if you want a Reading of the System Voltage UNcomment
/*
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}
*/
// Heat Index Function
//reference: http://en.wikipedia.org/wiki/Heat_index
double heatIndex(double Temperature, double Humidity)
{
  double c1 = -42.38, c2 = 2.049, c3 = 10.14, c4 = -0.2248, c5= -6.838e-3, c6=-5.482e-2, c7=1.228e-3, c8=8.528e-4, c9=-1.99e-6 ; 
  double T = Temperature;
  double R = Humidity;
  double T2 = T*T;
  double R2 = R*R;
  double TR = T*R;
  double rv = c1 + c2*T + c3*R + c4*T*R + c5*T2 + c6*R2 + c7*T*TR + c8*TR*R + c9*T2*R2;
  return rv;
}
// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm 
double dewPoint(double Temperature, double Humidity)
{
  double A0= 373.15/(273.15 + Temperature);
  double SUM = -7.90298 * (A0-1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
  SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM-3) * Humidity;
  double T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558-T);
}
RTC_DS1307 RTC;// Real Time Clock
//Ethernet card
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x00, 0xC1, 0xE1};
IPAddress ip(192,168,1,106);
EthernetClient client;
IPAddress server(64,94,18,121);  
unsigned long lastConnectionTime = 0; 
boolean lastConnected = false; 
const unsigned long postingInterval = 5 * 1000; // 5 Second Cosm Upload Interval adding to the delay's in the sketch

int Twc=0.0;	//Wind chill Temperature
//LDR LUX Reading info
int photocellPin = 3;            // the cell and 10K pulldown are connected to A3
//int Lux;                       // the analog reading from the sensor divider
int solarCell = 0;               // Solar cell on A0
//int Light;

void setup() 
{ 

  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  Serial.begin(115200);
  dht.begin();// Connect the DHT22 sensor
  RTC.begin();// Connect the RTC bob
  bmp.begin();// Connect the Baro/Tmp bob

  Serial.println("Arduino R3 Weather Station");
  Serial.println("BOOTING S.W. Ver 1.5 - Nov-30-2012");
  Serial.println("ip Address : ");
  Serial.print(ip); //show the local I.P. address for debugging
  Serial.println();
  Serial.print("-------------------");
  Serial.println();
  if (Ethernet.begin(mac) == 0) {
    Ethernet.begin(mac, ip);
  }
    if (tsl.begin()) {
    Serial.println("TSL2561 sensor Active!");
  } 
  else {
    Serial.println("TSL2561 sensor Malfunction?");
    while (1);
  }
    /*
    Uncomment line below and remove the RTC Battery 
   to set the RTC to the date & time this sketch was compiled
   Comment out soon after to use the RTC
   */

  //RTC.adjust(DateTime(__DATE__, __TIME__));  

  
  if (! RTC.isrunning()) {
    Serial.println("DS1307 'NOT' running!");
  }
  else {
    Serial.println("DS1307 'IS'  running!");
  }


  
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  //tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)

  // Now we're ready to get readings!
} 
void loop(){ 

  /*
       if there's incoming data from the net connection.
   send it out the serial port.  This is for debugging
   purposes only: (saves Terminal space)
   */
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    //Serial.println("DATA sent ---- disconnecting!."); 
    //Serial.println();
    client.stop();
  }
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    uint32_t lum = tsl.getFullLuminosity();
    uint16_t ir, full;
    
/*Temp	//1
Humi	//2
Light	//3
Baro	//4
HI	//5
Inside	//6
Dew	//7
wind	//8
Lux	//9
Chill	//10
Ir	//11
Visi	//12
Lux1	//13
Alt	//14
Cloud	//15
Time	//16
*/

    float Ir =          (ir = lum >> 16);
    float Full =        (full = lum & 0xFFFF);
    float Visi =        (Full - Ir);
    float Lux1 =        (tsl.calculateLux(Full, Ir));
    float Chill =       (Twc);// The Wind Chill
    float Lux =         analogRead(photocellPin);        //Custom Lux Reading for the sensor I have. Adjust your resistor to your needs.
    float Alt =         (bmp.readAltitude(102500)/3.3);    //Or....Replace the Figure in "(101325)" with 8.5'; Static altitude reading (Im at sea level)
    float Baro =        (bmp.readPressure()* 0.0002953); ;  //convert Pa to inches of Hg 
    float Inside =      (bmp.readTemperature)(); 
    float Humi =        (dht.readHumidity());              //Read the DHT humidity sensor
    float Temp =        (dht.readTemperature(2)); //Read the DHT Temp sensor (OutSide)
    float Light =       analogRead (solarCell) * 1.92 * 5.0 / 1023.0; //Read Solar Voltage Applied to Battery Pack (2x 220k resistors voltage divider)
    float Time =        millis() / 60000;                  //UpTime clock (x 60000 gives me a seconed
    float Dew =         (dewPoint(Temp, Humi)); //DHT22   
    float HI =          (heatIndex(Temp, Humi));//DHT22
    float wind =        (speedwind) / 0.445;            //Anemometer (NRG 40C) has 2 south poles,  2 pulses per revolution and / 0.445 to covert from m/s to MPH
    float Cloud =       ((Temp - Dew) / 4.4 * 10000 + Alt); // Cloud Base ref: http://en.wikipedia.org/wiki/Cloud_base
    windvelocity();
    Sample++;
    RPMcalc();
    WindSpeed();

    //WindChill
    if ((Temp <50.0) && (wind > 3.0))
    {
      Chill=35.74+0.6215*Temp-35.75*pow(wind,0.16)+0.4275*Temp*pow(wind,0.16);
    }
    else
    {
      Chill=Temp;
    }

    sendData(Temp, Humi, Light, Baro, HI, Inside, Dew, wind, Lux, Chill, Ir, Visi, Lux1, Alt, Time, Cloud);//, Sample
}
   
  lastConnected = client.connected();
}


void sendData(float Temp, float Humi, float Light, float Baro,  float HI, float Inside, 
float Dew, float wind, float Lux, float Chill, float Ir, float Visi, float Lux1, float Alt, float Time, float Cloud)
{
  //  
  if (client.connected()) client.stop();
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    Serial.println();
    Serial.println("Sending DATA!");


    client.print("PUT /v2/feeds/");
    client.print(FEEDID);
    client.println(".csv HTTP/1.1");
    client.println("Host: api.cosm.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    // Count the BRACKETS () and the letters in the word and add one for the number following the coma
    /*
Temp	//1
Humi	//2
Light	//3
Baro	//4
HI	//5
Inside	//6
Dew	//7
wind	//8
Lux	//9
Chill	//10
Ir	//11
Visi	//12
Lux1	//13
Alt	//14
Cloud	//15
Time	//16
*/    
    int length = 7 + countDigits(Temp,2)                          //1
      + 2 + 7 + countDigits(Humi,2)                               //2
        + 2 + 8 + countDigits(Light,2)                            //3
          + 2 + 7 + countDigits(Baro,2)                           //4
            + 2 + 5 + countDigits(HI,2)                           //5
              + 2 + 9 + countDigits(Inside,2)                     //6
                + 2 + 6 + countDigits(Dew,2)                      //7
                  + 2 + 7 + countDigits(wind,2)                   //8
                    + 2 + 6 + countDigits(Lux,2)                  //9
                      + 2 + 8 + countDigits(Chill,2)              //10
                        + 2 + 5 + countDigits(Ir,2)               //11
                          + 2 + 7 + countDigits(Visi,2)           //12
                            + 2 + 7 + countDigits(Lux1,2)         //13
                              + 2 + 6 + countDigits(Alt,2)        //14
                               + 2 + 8 + countDigits(Cloud,2)     //15
                                 + 2 + 7 + countDigits(Time,2);   //16

    client.println(length);
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();
    client.print("Outside,");    //01
    client.println(Temp);
    client.print("Humidity,");   //02
    client.println(Humi);
    client.print("Solar,");      //03
    client.println(Light);
    client.print("Baro,");       //04
    client.println(Baro);
    client.print("HeatIndex,");  //05
    client.println(HI);
    client.print("Inside,");     //06
    client.println(Inside);
    client.print("DewPoint,");   //07
    client.println(Dew);
    client.print("Wind,");       //08
    client.println(wind);
    client.print("Lux,");        //09
    client.println(Lux);
    client.print("WindChill,");  //10
    client.println(Chill);
    client.print("InfraRed,");   //11
    client.println(Ir);
    client.print("Visible,");    //12
    client.println(Visi);
    client.print("Lux1,");       //13
    client.println(Lux1);
    client.print("Alt,");        //14
    client.println(Alt);
    client.print("Cloud,");      //15
    client.println(Cloud);
    client.print("Time,");       //16
    client.println(Time);

  } 


  else {
    // if we couldn't make a connection to "Cosm.com" Serial the time of the occurance:
   /*
    DateTime now = RTC.now();//Read the RTC
    const uint8_t h = now.hour();
    const uint8_t hr_12 = h%12;
    Serial.print("connection failed at : ");
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

    Serial.println("disconnecting.");
    */
    client.stop();
  }
  lastConnectionTime = millis();


}
int countDigits(double number, int digits) { 
  int n = 0;
  if (number < 0.0)
  {
    n++; // "-";
    number = -number;
  }
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i) {
    rounding /= 10.0;
  }
  number += rounding;
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  while (int_part > 0) {
    int_part /= 10;
    n++;
  }
  if (digits > 0) {
    n++; //"."; 
  }
  while (digits-- > 0)
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    n ++; 
    remainder -= toPrint; 
  } 
  return n;
}

// Measure wind speed
void windvelocity(){
  speedwind = 0;
  counter = 0;  
  attachInterrupt(0, addcount, CHANGE);
  unsigned long millis();                     
  long startTime = millis();
  while(millis() < startTime + period) {
  }
  detachInterrupt(1);
}

void RPMcalc(){
  RPM=((counter*2)*60)/(period/1000);  // (added *2 insted of /2) Calculate revolutions per minute (RPM)
}
void WindSpeed(){
  speedwind = ((2 * pi * radio * RPM)/60) / 1000;  // Calculate wind speed on m/s
  if (speedwind <= 0.3){                           // Calculate Wind force depending of wind velocity
    windforce = 0;  // Calm
  }
  else if (speedwind <= 1.5){
    windforce = 1;  // Light air
  }
  else if (speedwind <= 3.4){
    windforce = 2;  // Light breeze
  }
  else if (speedwind <= 5.4){
    windforce = 3;  // Gentle breeze
  }
  else if (speedwind <= 7.9){
    windforce = 4;  // Moderate breeze
  }
  else if (speedwind <= 10.7){
    windforce = 5;  // Fresh breeze
  }
  else if (speedwind <= 13.8){
    windforce = 6;  // Strong breeze
  }
  else  if (speedwind <= 17.1){
    windforce = 7;  // High wind, Moderate gale, Near gale
  }
  else if (speedwind <= 20.7){
    windforce = 8;  // Gale, Fresh gale
  }
  else if (speedwind <= 24.4){
    windforce = 9;  // Strong gale
  }
  else if (speedwind <= 28.4){
    windforce = 10;  // Storm, Whole gale
  }
  else if (speedwind <= 32.6){
    windforce = 11;  // Violent storm
  }
  else {
    windforce = 12;  // Hurricane (from this point, apply the Fujita Scale)
  }
}
void addcount(){
  counter++;
}











