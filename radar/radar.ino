// NewPing Library - v1.5 - 08/15/2012
//
// AUTHOR/LICENSE:
// Created by Tim Eckel - teckel@leethost.com
// Copyright 2012 License: GNU GPL v3 http://www.gnu.org/licenses/gpl
// ---------------------------------------------------------------------------
// Example NewPing library sketch that does a ping about 20 times per second.
// ---------------------------------------------------------------------------

#include <NewPing.h>
#include <Servo.h> 
#define TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
Servo myservo;  // create servo object to control a servo 
// a maximum of eight servo objects can be created 

int pos = 0;    // variable to store the servo position

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

void setup() {
  Serial.begin(9600); // Open serial monitor at 115200 baud to see ping results.
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object 

}

void loop() {



  for(pos = 0; pos < 179; pos += 1)  // goes from 0 degrees to 180 degrees 
  {  
    delay(50);                      // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
    unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
   // Serial.println("");
    // Convert ping time to distance in cm and print result (0 = outside set distance range)
    //Serial.println(" inch's");
    // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(50);    // waits 50ms for the servo to reach the position 
    Serial.print("X");
    Serial.print(pos);   
    Serial.print("V");
    Serial.println(uS / US_ROUNDTRIP_CM);


    //delay(100);
  } 
  for(pos = 179; pos>=1; pos-=1)     // goes from 180 degrees to 0 degrees 
  {    
    delay(50);                      // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
    unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(50);    // waits 15ms for the servo to reach the position 
    //Serial.println("");
    Serial.print("X"); // this is what the processing app is looking for to mark Posistion
    Serial.print(pos); // this is what the processing app is looking for to mark Posistion
    Serial.print("V"); // this is what the processing app is looking for to mark Distance
    Serial.println(uS / US_ROUNDTRIP_CM);  // this is what the processing app is looking for to mark Distance

    //delay(100);
  }

}




