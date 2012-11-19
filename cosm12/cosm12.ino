/*
  Cosm sensor client
 
 This sketch connects an analog sensor to Cosm (http://www.cosm.com)
 using a Wiznet Ethernet shield. You can use the Arduino Ethernet shield, or
 the Adafruit Ethernet shield, either one will work, as long as it's got
 a Wiznet Ethernet module on board.
 
 This example has been updated to use version 2.0 of the cosm.com API. 
 To make it work, create a feed with a datastream, and give it the ID
 sensor1. Or change the code below to match your feed.
 
 
 Circuit:
 * Analog and Digital sensors attached as well as I2C
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 created 15 March 2010
 updated 14 May 2012
 by Tom Igoe with input from Usman Haque and Joe Saavedra
 Other Authors -
 M.A. de Pablo & C. de Pablo S., 2010: Anemometer,
 ADAfruit: BMP085, RTC, DHT22 
 Tweeked by ME :-)
http://arduino.cc/en/Tutorial/CosmClient
 This code is in the public domain.
 
 */

//Binary sketch size: 29,008 bytes (of a 32,256 byte maximum) without Serial 27,290 bytes :-)

#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include <DHT.h>
#include <Ethernet.h>
#include <Adafruit_BMP085.h>

#define ANEMOMETER 2 
#define DHTTYPE DHT22 
#define DHTPIN (A2) 

//COSM info
#define APIKEY "Your API key" // your cosm api key
#define FEEDID Your feed ID   // feed ID
#define USERAGENT "Cosm Arduino Example (Your feed ID )" // user agent is the project name
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
  "Calm", "Light air", "Light breeze", "Gentle breeze", "Moderate breeze", "Fresh breeze", "Strong breeze", "Moderate gale", "Fresh gale", "Strong gale", "Storm", "Violent storm", "Hurricane"};

// Variable definitions for Anemometer

unsigned int Sample = 0;       // Sample number
unsigned int counter = 0;      // B/W counter for sensor 
unsigned int RPM = 0;          // Revolutions per minute
float speedwind = 0;           // Wind speed (m/s)
unsigned short windforce = 0;  // Beaufort Wind Force Scale

// Reading the System Voltage
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
const unsigned long postingInterval = 5*1000; // 5 Second Cosm Upload Interval adding to the delay's in the sketch

int Twc=0.0;	//Wind chill Temperature
//LDR LUX Reading info
int photocellPin = 3;          // the cell and 10K pulldown are connected to Analog pin 3
int Lux;                       // the analog reading from the sensor divider
int solarCell = 0;
int Light;

void setup() 
{ 

  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  Serial.begin(115200);
  dht.begin();// Connect the DHT22 sensor
  RTC.begin();// Connect the RTC bob
  bmp.begin();// Connect the Baro/Tmp bob

  Serial.print("Arduino R3 Weather Station");
  Serial.println();
  Serial.print("BOOTING S.W. Ver 1.5 - Oct-2012");
  Serial.println();
  Serial.print("ip Address : ");
  Serial.println(ip); //show the local I.P. address for debugging
  Serial.print("");
  Serial.print("-------------------");
  Serial.println();
  if (Ethernet.begin(mac) == 0) {
    Ethernet.begin(mac, ip);
  }
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    
    /*
    Uncomment line below and remove the RTC Battery 
    to set the RTC to the date & time this sketch was compiled
    Comment out soon after to use the RTC
    */
    
    //RTC.adjust(DateTime(__DATE__, __TIME__));  
  }

} 
void loop(){ 
      /*
       if there's incoming data from the net connection.
       send it out the serial port.  This is for debugging
       purposes only: (saves Terminal space)
      */
  //if (client.available()) {
      // char c = client.read();
      //Serial.print(c);
 // }
 
   // if there's no net connection, but there was one last time
   // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    //Serial.println("disconnecting."); 
    //Serial.println();
    client.stop();
  }
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    float WindChill =   (Twc);
    float Lux =         analogRead(photocellPin);        //Custom Lux Reading for the sensor I have. Adjust yours to your needs.
    float Alt =         (bmp.readAltitude(102425)/3.3);    //Or....Replace the Figure in "(101325)" with 8.5'; Static altitude reading (Im at sea level)
    float Baro =        (bmp.readPressure()) * 0.0002953;  //convert Pa to inches of Hg 
    float Inside =      (bmp.readTemperature)(); 
    float Humidity =    (dht.readHumidity());              //Read the DHT humidity sensor
    float Temperature = (dht.readTemperature(2)); //Read the DHT Temp sensor (OutSide)
    float Light =       analogRead (solarCell) * 1.92 * 5.0 / 1023.0; //Read Solar Voltage Applied to Battery Pack (2x 220k resistors voltage divider)
    float Time =        millis() / 60000;                  //UpTime clock (x 60000 gives me a seconed
    float Dew =         (dewPoint(Temperature, Humidity)); //DHT22   
    float HI =          (heatIndex(Temperature, Humidity));//DHT22
    float wind =        (speedwind) / 0.445;            //Anemometer (NRG 40C) has 2 south poles,  2 pulses per revolution and / 0.445 to covert from m/s to MPH
    windvelocity();
    Sample++;
    RPMcalc();
    WindSpeed();

    //WindChill
    if ((Temperature <50.0) && (wind > 3.0))
    {
      WindChill=35.74+0.6215*Temperature-35.75*pow(wind,0.16)+0.4275*Temperature*pow(wind,0.16);
    }
    else
    {
      WindChill=Temperature;
    }

    //Next: Read the RTC
    DateTime now = RTC.now();
    const uint8_t h = now.hour();
    const uint8_t hr_12 = h%12;

    //29,008 with Serial Output ? without 27,310 use to debug or send it to your monitor for up to the second weather
/*
    Serial.println();
    Serial.print("Time        : ");
    if(hr_12 < 10){                                       // Zero padding if value less than 10 ie."09" instead of "9"
      Serial.print(" ");
      Serial.print((h > 12) ? h - 12 : ((h == 0) ? 12 : h), DEC); // Conversion to AM/PM  
    }
    else{
      Serial.print((h > 12) ? h - 12 : ((h == 0) ? 12 : h), DEC); // Conversion to AM/PM
    }
    Serial.print(':');
    if(now.minute() < 10){                                // Zero padding if value less than 10 ie."09" instead of "9"
      Serial.print("0");
      Serial.print(now.minute(), DEC);
    }
    else{
      Serial.print(now.minute(), DEC);
    }
    if(h < 12){                                           // Adding the AM/PM sufffix
      Serial.print(" AM");
    }
    else{
      Serial.print(" PM");
    }
    Serial.print("");
    Serial.println();
    Serial.print("Date        : " );
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.println();
    //Serial.print(" Starting measurements...");
    //Serial.println(" finished.");
    Serial.print("Sample#     : ");
    Serial.println(Sample);    
    Serial.print("Counter     : ");
    Serial.println(counter);
    Serial.print("RPM         : ");
    Serial.println(RPM);
    Serial.print("Wind speed  : ");
    Serial.println(wind); // pin (D2)
    Serial.print("Wind force  : ");
    Serial.println(winds[windforce]);
    Serial.print("System Volts: ");
    Serial.println(readVcc() / 1000. , DEC);
    Serial.print("Solar Volts : ");
    Serial.print(Light) ;
    if (Light  <1.0) {
      Serial.println(" - Night Time");
    }
    else if (Light  >4.8){
      Serial.println (" - Charging");
    }
    else{
      Serial.println("");
    }
    Serial.print("Altitude    : ");
    Serial.println(Alt);
    Serial.print("Baro        : ");
    Serial.println(Baro);
    Serial.print("Humidity    : ");
    Serial.println(Humidity);
    Serial.print("heatIndex   : ");
    Serial.println(HI);
    Serial.print("Outside     : ");
    Serial.println(Temperature);
    Serial.print("WindChill   : ");
    Serial.println(WindChill);
    Serial.print("Dew Point   : ");
    Serial.println(Dew);
    Serial.print("Inside      : ");
    Serial.println(Inside);
    Serial.print("Lux         : ");
    Serial.print(Lux);//pin (A3)
   if (Lux < 5) {
      Serial.println(" Star Glow");
    } 
    else if (Lux < 10) {
      Serial.println("  Night Time");
    } 
    else if (Lux < 200) {
      Serial.println("  Dim");
    }
    else if (Lux < 455) {
      Serial.println(" Sunrise/Sunset");
    }     
    else if (Lux < 500) {
      Serial.println("  Light");
    } 
    else if (Lux < 800) {
      Serial.println("  Bright");
    } 
    else {
      Serial.println("  Very bright");
    }
    Serial.println();
    Serial.print("-------------------");
    */

    sendData(Time, Temperature, Humidity, Inside, Light, Baro,
    HI, Alt, Dew, wind, Lux, WindChill); //, Sample);
  }
  lastConnected = client.connected();
}
void sendData( float Time, float Temperature, float Humidity, 
float Inside, float Light, float Baro, float HI, float Alt, 
float Dew, float wind, float Lux, float WindChill){//, float Sample

  if (client.connected()) client.stop();
  if (client.connect(server, 80)) {

    client.print("PUT /v2/feeds/");
    client.print(FEEDID);
    client.println(".csv HTTP/1.1");
    client.println("Host: api.cosm.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    int length = 5 + countDigits(Time,2) + 9 + 12 + countDigits(Temperature,2)
      + 2 + 9 + countDigits(Humidity,2) + 2 + 6 + countDigits(Light,2)
        + 2 + 5 + countDigits(Baro,2) + 2 + 3 + countDigits(HI,2)
          + 2 + 7 + countDigits(Inside,2) + 2 + 4 + countDigits(Alt,2)
            + 2 + 4 + countDigits(Dew,2) + 2 + 5 + countDigits(wind,2)
              + 2 + 4 + countDigits(Lux,2) + 2 + 10 + countDigits(WindChill,2);// + 2 + 7 + countDigits(Sample,2)
    client.println(length);
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();
    client.print("Wind,");
    client.println(wind);
    client.print("Alt,");
    client.println(Alt);
    client.print("Baro,");
    client.println(Baro);
    client.print("DewPoint,");
    client.println(Dew);
    client.print("HeatIndex,");
    client.println(HI);
    client.print("Humidity,");
    client.println(Humidity);
    client.print("Inside,");
    client.println(Inside);
    client.print("Outside,");
    client.println(Temperature);
    client.print("Solar,");
    client.println(Light);
    client.print("Time,");
    client.println(Time);
    client.print("Lux,");
    client.println(Lux);
    client.print("WindChill,");
    client.println(WindChill);
    //Sample++;
    //client.print("Sample,");
    //client.println(Sample);
  } 


  else {
    // if we couldn't make a connection to "Cosm.com" Serial the time of the occurance:
    DateTime now = RTC.now();//Read the RTC
    const uint8_t h = now.hour();
    const uint8_t hr_12 = h%12;

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










