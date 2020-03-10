/*
description:     Rain Gauge with arduino with serial monitoring
                 Reports the daily-rain and rain-in-last-hour in inches
*/
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "RTClib.h"
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <Wire.h>
#include <BlynkSimpleEsp8266.h>

//#define RainPin 2                         // The Rain input is connected to digital pin 2 on the arduino
#define FIREBASE_HOST "rain-gauge-a9fdf.firebaseio.com"
#define FIREBASE_AUTH "e5UMKhUYC8LsyBmhHZR3F12xEQDgeMr8FTNUIDcq"
#define WIFI_SSID "#1  d^__^b"
#define WIFI_PASSWORD "oporayam2k17"

bool bucketPositionA = false;             // one of the two positions of tipping-bucket               
const double bucketAmount = 0.053;        // 0.053 inches atau 1.346 mm of rain equivalent of ml to trip tipping-bucket 

//note:
//diketahui bahwa 1 inchi = 2.54 cm
//diketahui bahwa 1 tip sensor ini = 2.6 mL
//maka 0.053 inchi of rain didapat dari:
//panjang rain collector = 5.4 cm atau 2.126 inchi
//lebar rain collector = 3.6 cm atau 1.417 inchi
//luas=pxl -> 2.16 inchi x 1.417 inchi = 3.012 inchi persegi
//U.S. measures rain in inches so it would be 3.012 inchi kubik
//lalu dikonversi dari inchi kubik mjd mL dan didapat bahwa 3.012 inchi kubik = 49.358 mL
//yg artinya 1 inchi of rain = 49.358 mL
//sehingga 1 tip sensor ini mewakili 2.6 mL/49.358 mL = 0.053 inchi of rain
 
double dailyRain = 0.0;                   // rain accumulated for the day
double hourlyRain = 0.0;                  // rain accumulated for one hour
double dailyRain_till_LastHour = 0.0;     // rain accumulated for the day till the last hour          
bool first;                               // as we want readings of the (MHz) loops only at the 0th moment 

RTC_Millis rtc;                           // software RTC time

BlynkTimer timer;
void uploadData(){
  StaticJsonBuffer<200> jbLog, jbDate;
  JsonObject& date = jbDate.createObject();
  date[".sv"] = "timestamp";

  JsonObject& Log = jbLog.createObject();
  Log["dailyRain"] = (dailyRain*2.54*10);
  Log["hourlyRain"] = (hourlyRain*2.54*10); 
  Log["date"] = date;

  Serial.println("Sending (writing)");
  Log.prettyPrintTo(Serial);
//  Serial.printf("\nto /user/%s\n", newlyFoundCardUID.c_str());

   String pushID = Firebase.push("logs", Log);
  // handle error
  if (Firebase.failed()) {
      Serial.print("pushing /logs failed:");
      Serial.println(Firebase.error());  
      return;
  }
  Serial.print("pushed: /logs/");
  Serial.println(pushID);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup(void) {
  Serial.begin(9600);                            // start the serial port
  rtc.begin(DateTime(__DATE__, __TIME__));       // start the RTC
  pinMode(D5, INPUT);                       // set the Rain Pin as input.
  delay(1000);                                   // wait the serial monitor 
  Serial.println("Rain Gauge Ready !!");         // rain gauge measured per 1 hour           
  Serial.println("execute calculations once per hour !!");

//  WiFi.disconnect();
  
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
    if(!wifiManager.autoConnect("Wemos D1")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  Serial.println();
  Serial.print("connected to internet: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timer.setInterval(60000L, uploadData);
}


void loop(void){
  DateTime now = rtc.now();
    
  // ++++++++++++++++++++++++ Count the bucket tips ++++++++++++++++++++++++++++++++
  if ((bucketPositionA==false)&&(digitalRead(D5)==LOW)){
    bucketPositionA=true;
    dailyRain+=bucketAmount;                               // update the daily rain
  }
  
  if ((bucketPositionA==true)&&(digitalRead(D5)==HIGH)){
    bucketPositionA=false;  
  } 
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  if(now.minute() != 0) first = true;                     // after the first minute is over, be ready for next read
  
  if(now.minute() == 0 && first == true){
 
    hourlyRain = dailyRain - dailyRain_till_LastHour;      // calculate the last hour's rain 
    dailyRain_till_LastHour = dailyRain;                   // update the rain till last hour for next calculation
    
    // fancy display for humans to comprehend
    Serial.println();
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":  Total Rain for the day = ");
    Serial.print(dailyRain,3);                            // the '3' ensures the required accuracy digit dibelakang koma
    Serial.print(" inches atau ");
    Serial.print(dailyRain*2.54*10,3);
    Serial.println(" mm");
    Serial.println();
    Serial.print("     :  Rain in last hour = ");
    Serial.print(hourlyRain,3); 
    Serial.print(" inches atau ");
    Serial.print(hourlyRain*2.54*10,3);
    Serial.println(" mm"); 
    Serial.println();
    
    first = false;                                        // execute calculations only once per hour
  }
  
  if(now.hour()== 0) {
    dailyRain = 0.0;                                      // clear daily-rain at midnight
    dailyRain_till_LastHour = 0.0;                        // we do not want negative rain at 01:00
  }
   timer.run();
}                                                        // end of loop
