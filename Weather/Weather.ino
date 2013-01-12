


//Binary sketch size: 29,942 bytes with Serial output (of a 32,256 byte maximum)
// 01/01/13 Changed the integration time for the Light Sensor 
// Cosm sensor client

#include <Wire.h>
#include <TSL2561.h>
#include "RTClib.h"
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_BMP085.h>
#include "TSL2561.h"

#define ANEMOMETER 2 
#define DHTTYPE DHT22 
#define DHTPIN (A2) 
#define APIKEY "" // your cosm api key
#define FEEDID 72737   // feed ID
#define USERAGENT "Cosm Arduino Example (72737 )" // user agent is the project name
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
TSL2561 tsl(TSL2561_ADDR_FLOAT); 

// Variable definitions for Temperature
int SampleT; // How many samples do we have
long runningT; // Sum of all speeds
int highT; // Top sp

int Twc = 0.0;	//Wind chill Temperature
int photocellPin = 3;            // the cell and 10K pulldown are connected to A3
int solarCell = 0;

// Constants definitions for Anemometer
const float coeff = 0.10;//  //coefficient for exponential running ave
const float pi = 3.14159265; // pi number (dont use for delivery!)
int period = 3000;           // Measurement period (miliseconds)
int delaytime = 3000;        // Time between samples (miliseconds)
int radio = 70;              // NRG40c (70) Radius from vertical anemometer axis to a cup center (mm)
char* winds[] = {
  "Calm", "Light air", "Light breeze", "Gentle breeze", "Moderate breeze", "Fresh breeze", "Strong breeze", "Moderate gale", "Fresh gale", "Strong gale", "Storm", "Violent storm", "AHHHHHHHH!   Hurricane"};
// Variable definitions for Anemometer averaging
int Sample; // How many samples do we have
long runningtotal; // Sum of all speeds
int maxwind; // Top speed

// Variable definitions for Anemometer
//unsigned int Sample = 0;       // Sample number
unsigned int counter = 0;      // B/W counter for sensor 
unsigned int RPM = 0;          // Revolutions per minute
float speedwind = 0;           // Wind speed (m/s)
unsigned short windforce = 0;  // Beaufort Wind Force Scale
float avg; //variable for average mph
float avgT;//variable for Temp
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
//Ethernet card
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x00, 0xC1, 0xE1};
IPAddress ip(192,168,1,106);
EthernetClient client;
IPAddress server(64,94,18,121);  
unsigned long lastConnectionTime = 0; 
boolean lastConnected = false; 
const unsigned long postingInterval = 5 * 1000; // 5 Second Cosm Upload Interval adding to the delay's within the sketch


void setup() 
{ 

   tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  //tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
   tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
   //pinMode(13, OUTPUT);
  Serial.begin(57600);
  dht.begin();// Connect the DHT22 sensor
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
  //if (tsl.begin()) {
   // Serial.println("TSL2561 sensor Active!");
  //} 
  //else {
  //  Serial.println("TSL2561 sensor Malfunction?");
   // while (1);
  //}
} 
void loop(){ 

/*
       if there's incoming data from the net connection.
   send it out the serial port.  This is for debugging
   purposes only: (saves Terminal space)
   */
  if (client.available()) {
    char c = client.read();
    //Serial.print(c);
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

    
    float Ir =          (ir = lum >> 16);
    float Full =        (full = lum & 0xFFFF);
    float Visi =        (Full - Ir);
    float Lux1 =        (tsl.calculateLux(Full, Ir));
    float Chill =       (Twc);// The Wind Chill
    float Lux =          analogRead(photocellPin);        //Custom Lux Reading for the sensor I have exposed to direct sun light.
    float Alt =         (bmp.readAltitude(102500)/3.3);    //convert meters to feet and, Or.. Replace the Figure in "(101325)" with 8.5' for Static altitude reading. (Im at sea level)
    float Baro =        (bmp.readPressure()* 0.0002953);   //convert Pa to inches of Hg 
    float Inside =      (bmp.readTemperature)(); 
    float Humi =        (dht.readHumidity());              //Read the DHT humidity sensor
    float Temp =        (dht.readTemperature(2)); //Read the DHT Temp sensor (OutSide)
    float Light =        analogRead (solarCell) * 6.92 / 1023.0; //Read Solar Voltage Applied to Battery Pack (2x 220k resistors voltage divider)
    float Time =         millis() / 60000;                  //UpTime clock (x 60000 gives me a seconed
    float Dew =         (dewPoint(Temp, Humi)); //DHT22   
    float HI =          (heatIndex(Temp, Humi));//DHT22
    float wind =        (speedwind) / 0.445;            //Anemometer (NRG 40C) has 2 south poles,  2 pulses per revolution and / 0.445 to covert from m/s to MPH
    float Cloud =       ((Temp - Dew) / 4.4 * 10000 + Alt); // Cloud Base ref: http://en.wikipedia.org/wiki/Cloud_base
    windvelocity();
    Sample++;
    RPMcalc();
    WindSpeed();
    avg = wind * coeff + avg * (1 - coeff); //calculation for average windspeed
    
    avgT = Temp * coeff + avgT  * (1 - coeff) ; //calculation for average Temp

    // For Temp MIn/Max/AVG
    int curT = Temp; // Get Temp
    if (curT > highT) {
      highT = curT;
    }
    runningT += curT;
    SampleT++;
    //For wind Min/Max/AVG
    int curspeed = wind; // Get wind speed
    if (curspeed > maxwind) {
      maxwind = curspeed;
    }
    runningtotal += curspeed;
    Sample++;


    //WindChill
    if ((Temp <50.0) && (wind > 3.0))
    {
      Chill=35.74+0.6215*Temp-35.75*pow(wind,0.16)+0.4275*Temp*pow(wind,0.16);
    }
    else
    {
      Chill=Temp;
    }
    Serial.println("----------");
    Serial.print("AVG Temp:   ");
    Serial.println  (avgT);
    Serial.print("High:       ");
    Serial.println (highT);
    Serial.println("----------");
    //wind
    Serial.print("AVG Wind:   ");
    Serial.println (avg);
    Serial.print("maxwind:   ");
    Serial.println (maxwind);
    Serial.println("----------");
     
    Serial.print("Alt:        ");        //01
    Serial.println (Alt);
    Serial.print("Baro:       ");        //02
    Serial.println  (Baro);
    Serial.print("Cloud:      ");        //03
    Serial.println  (Cloud);
    Serial.print("DewPoint:   ");        //04
    Serial.println  (Dew);
    Serial.print("Full:       ");        //05
    Serial.println  (Full);
    Serial.print("HeatIndex:  ");        //06
    Serial.println  (HI);
    Serial.print("Humidity:   ");        //07
    Serial.println  (Humi);
    Serial.print("InfraRed:   ");        //08
    Serial.println  (Ir);
    Serial.print("TempI:      ");        //09
    Serial.println(Inside);
    Serial.print("TempO:      ");       //10
    Serial.println(Temp);
    Serial.print("Solar:      ");       //11
    Serial.println(Light);
    Serial.print("Wind:       ");       //12
    Serial.println(wind);
    Serial.print("Wind avg:   ");
    Serial.println(avg);    
    Serial.print("Lux:        ");       //13
    Serial.println(Lux);
    Serial.print("WindChill:  ");       //14
    Serial.println(Chill);
    Serial.print("Visible:    ");       //15
    Serial.println  (Visi);
    Serial.print("Lux1:       ");       //16
    Serial.println  (Lux1);
    Serial.print("Time:       ");       //17
    Serial.println(Time);

    sendData(Temp, Humi, Light, Baro, HI, Inside, Dew, wind, Lux, Chill, Ir, Visi, Lux1, Alt, Time, Cloud, Full, avg, avgT, maxwind, highT);
  }

  lastConnected = client.connected();
}

void sendData(float Temp, float Humi, float Light, float Baro,  float HI, float Inside, float Dew, float wind, float Lux, float Chill,
float Ir, float Visi, float Lux1, float Alt, float Time, float Cloud, float Full, float avg, float avgT, float maxwind, float highT)
{
  
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
    // Count the BRACKETS "(" ")" and the letters in the word and add one for the number "2" following the coma
    int length = 7 + countDigits(Temp,2)                           //1
      + 2 + 7 + countDigits(Humi,2)                                //2
        + 2 + 8 + countDigits(Light,2)                             //3
          + 2 + 7 + countDigits(Baro,2)                            //4
            + 2 + 5 + countDigits(HI,2)                            //5
              + 2 + 9 + countDigits(Inside,2)                      //6
                + 2 + 6 + countDigits(Dew,2)                       //7
                  + 2 + 7 + countDigits(wind,2)                    //8
                    + 2 + 6 + countDigits(Lux,2)                   //9
                      + 2 + 8 + countDigits(Chill,2)               //10
                        + 2 + 5 + countDigits(Ir,2)                //11
                          + 2 + 7 + countDigits(Visi,2)            //12
                            + 2 + 7 + countDigits(Lux1,2)          //13
                              + 2 + 6 + countDigits(Alt,2)         //14
                                + 2 + 8 + countDigits(Cloud,2)     //15
                                  + 2 + 7 + countDigits(Time,2)    //16
                                    + 2 + 7 + countDigits(Full,2)  //17
                                      + 2 + 6 + countDigits(avg,2)//18
                                       + 2 + 7 + countDigits(avgT,2)
                                         + 2 + 10 + countDigits(maxwind,2)
                                          + 2 + 8 + countDigits(highT,2);
    client.println(length);
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();
    client.print("TempOut,");    //01
    client.println(Temp);
    client.print("Humidity,");   //02
    client.println(Humi);
    client.print("Solar,");      //03
    client.println(Light);
    client.print("Baro,");       //04
    client.println(Baro);
    client.print("HeatIndex,");  //05
    client.println(HI);
    client.print("TempIn,");     //06
    client.println(Inside);
    client.print("DewPoint,");   //07
    client.println(Dew);
    client.print("Wind,");       //08
    client.println(wind);
    client.print("WindAVG,");       //08
    client.println(avg);  
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
    client.print("Full,");       //17
    client.println(Full);
    client.print("TempAVG,");
    client.println(avgT);
    client.print("WindMAX,");
    client.println(maxwind);   
    client.print("TempMAX,");
    client.println(highT); 
  } 
  else {
    Serial.println("Something Happened !");
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

//Measure RPM
void RPMcalc(){
  RPM=((counter*2)*60)/(period/1000);  // Added *2 insted of /2 (four pole magnet) Calculate revolutions per minute (RPM)
}
// select text for display based on windspeed
void WindSpeed(){
  speedwind = ((2 * pi * radio * RPM)/60) / 1000;  // Calculate wind speed on m/s

}

// show active pulses 
void addcount(){
  counter++;
}





















