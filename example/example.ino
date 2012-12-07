




/*
  Cosm sensor client
  -------------------------------------------------------------------------------
 This sketch hopes to help othes in connecting more than one device to COSM.com Using Anolog devices and I2C devices
 at the same time.
 You can add as many analog devices as you want or (have Analog reading pins available on an Arduino UNO R3)-
- and as many I2C devices as long as the addresses dont conflict.
 Specialty devices that need specialty Libraries will tax your Micro controllers memory. So think your project out and Verify it!
  -------------------------------------------------------------------------------
 This project/Sketch uses 3 basic commonly available Sensor's...
 The first is the most basic..  An Analog sensor to give a basic voltage or resitive Value, In this case a CdS, or PhotoCell.(A3)
 The Second is an Analog device that puts out Digital data to an Analog pin (A2) in this case a DHT22 Temperature and Humidity sensor
 The third uses i2c SDA data on analog(A4) and  i2c SCL clock data on analog(A5) on Arduino UNO.
  -------------------------------------------------------------------------------
 This sketch connects a cdS sensor  https://www.adafruit.com/products/161
 A DHT 11/22 Temp/Humidity Sensor https://www.adafruit.com/products/385 
 and a BMP085 I2C Sensor https://www.adafruit.com/products/391 
 On an Arduino UNO and Arduino Ethernet Shield to connect your I.O.T. to Cosm.
 You can use the Arduino Ethernet shield, or the Adafruit Ethernet shield, either one will work, as long as it's got
 a Wiznet Ethernet module on board.
  -------------------------------------------------------------------------------
 This example has been updated to use version 2.0 of the cosm.com API. 
 To make it work, change the code below to match your feed.
 
 
 Circuit:
 * All Sensors are POWERED BY 3.3V
 * Analog sensor "cdS"(PhotoCell)  https://www.adafruit.com/products/161 attached to analog in (A3)
 * DHT 11 or 22 https://www.adafruit.com/products/385 attached to analog in (A2)
 * BMP085  https://www.adafruit.com/products/391 attached to analog in (A4) and analog (A5) (I2C)
 * Ethernet shield attached to pins 10, 11, 12, 13
  -------------------------------------------------------------------------------
 created 15 March 2010
 updated 14 May 2012
 by Tom Igoe with input from Usman Haque and Joe Saavedra
  -------------------------------------------------------------------------------
http://arduino.cc/en/Tutorial/CosmClient
 This code is in the public domain.
 
 */
 
#include <Wire.h> //Library for the I2C bmp085
#include <SPI.h> //Library For the Communication to the Ethernet Card
#include <Ethernet.h>//Ethernet Card Library
#include <DHT.h> // DH11,DHT22 and AM2302 (wired DHT22) series Library
#include <Adafruit_BMP085.h>// Adafruit_BMP085 Library Modified (https://github.com/fmbfla/Arduino/tree/master/lib's/ADbmp085)
#define DHTTYPE DHT22 //If you have a DHT11 change the "22" to "11" otherwise leave it
#define DHTPIN (A2) // Pin the DHT sensor is connected to
#define APIKEY         "YOUR API KEY GOES HERE" // replace your Cosm api key here
#define FEEDID          "My Project"// replace your feed ID
#define USERAGENT      "My Project" // user agent is the project name
//DHT info
DHT dht(DHTPIN, DHTTYPE);
//BARO/TEMP info
Adafruit_BMP085 bmp; 


int photocellPin = 3;            // the CdS cell and 10K pulldown are connected to analog(A3)https://www.adafruit.com/products/161

// Assign a MAC address for the ethernet controller.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// fill in your address here:
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0xC1, 0xE1};

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip(192,168,1,106);

// initialize the library instance:
EthernetClient client;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(216,52,233,121);      // numeric IP for api.cosm.com
IPAddress server(64,94,18,121);   // name address for cosm API

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
const unsigned long postingInterval = 10*1000; //delay between updates to cosm.com


void setup() {
  // start serial port:
  Serial.begin(9600);
  bmp.begin();// BMP085
  dht.begin();// DHT22


 // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip);
  }
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
    float Lux =         analogRead(photocellPin);        //Custom Lux Reading for the sensor I have to read moon light, star glow. Adjust your resistor to your needs.
    float Alt =         (bmp.readAltitude(101800));     // The library I use is MODIFIED... Or....Replace the Figure in "(101325)" with 8.5'; Static altitude reading (Im at sea level)
    float Baro =        (bmp.readPressure()* 0.0002953); ;  //convert Pa to inches of Hg 
    float Inside =      (bmp.readTemperature)(); // The library I use is MODIFIED
    float Humi =        (dht.readHumidity());              //Read the DHT humidity sensor
    float Temp =        (dht.readTemperature(2)); //Read the DHT Temp sensor (OutSide)
    
   // Serial data for Debugging, Comment out if not needed............ 
    
    Serial.print("Altitude    : ");
    Serial.println(Alt);
    Serial.print("Barometer        : ");
    Serial.println(Baro);
    Serial.print("Temperature     : ");
    Serial.println(Temp);
    Serial.print("Humidity    : ");
    Serial.println(Humi);
    Serial.print("Light         : ");
    Serial.print(Lux);
    sendData(Temp, Humi, Lux, Alt, Baro);//, Sample
}
   
  lastConnected = client.connected();
}


void sendData(float Temp, float Humi, float Lux, float Alt, float Baro)
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
    // Count the BRACKETS () and the letters in the words you use and add one for the number following the coma
    int length = 7 + countDigits(Temp,2)//= 7
      + 2 + 7 + countDigits(Humi,2) //= 7
       + 2 + 6 + countDigits(Lux,2) // = 6
        + 2 + 6 + countDigits(Alt,2) // = 6
          + 2 + 7 + countDigits(Baro,2); // = 7
       client.println(length);
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();
    //---------------------------------
    client.print("Alt,");        
    client.println(Alt);
    client.print("Temperature,");    
    client.println(Temp);
    client.print("Humidity,");   
    client.println(Humi);
    client.print("Light,");      
    client.println(Lux);
    client.print("Baro,");       
    client.println(Baro);
    } 


  else {
    // if we couldn't make a connection to "Cosm.com" Serial the time of the occurance:

    Serial.println();

    Serial.println("disconnecting.");
    
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
