/**
   DayWOBug.ino

    Created on: 04.10.2025

*/

#include <Arduino.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "Secrets.h"


String urlOpenBugs = "https://fupgmbh.atlassian.net/rest/api/3/search/jql?jql=project = JOYclub AND component = Team-Plasma AND issuetype = Bug AND statusCategory not in (Done)&startAt=0&maxResults=1&fields=created,resolutiondate"; 
String urlDoneBugs = "https://fupgmbh.atlassian.net/rest/api/3/search/jql?jql=project = JOYclub AND component = Team-Plasma AND issuetype = Bug AND resolved is not EMPTY ORDER BY resolutiondate DESC&startAt=0&maxResults=1&fields=created,resolutiondate"; 

WiFiUDP ntpUDP; 
NTPClient timeClient(ntpUDP, "pool.ntp.org");

ESP8266WiFiMulti WiFiMulti;

const int LED_T = 13; //top
const int LED_TL = 14; //top left
const int LED_TR = 15; //top right
const int LED_M = 0; //middle
const int LED_LL = 5; //lower left
const int LED_LR = 4; //lower right
const int LED_L = 16;  //lower
const int LED_P = 12; //dot


void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  pinMode(LED_T, OUTPUT);
  pinMode(LED_TL, OUTPUT);
  pinMode(LED_TR, OUTPUT);
  pinMode(LED_M, OUTPUT);
  pinMode(LED_LL, OUTPUT);
  pinMode(LED_LR, OUTPUT);
  pinMode(LED_L, OUTPUT);
  pinMode(LED_P, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_T, LOW);
  digitalWrite(LED_TL, LOW);
  digitalWrite(LED_TR, LOW);
  digitalWrite(LED_M, LOW);
  digitalWrite(LED_LL, LOW);
  digitalWrite(LED_LR, LOW);
  digitalWrite(LED_L, LOW);
  digitalWrite(LED_P, LOW);
  digitalWrite(LED_BUILTIN, HIGH);



  printChar("x");
  /*
  delay(500); 
  printChar("1");
  delay(500); 
  printChar("2");
  delay(500); 
  printChar("3");
  delay(500); 
  printChar("4");
  delay(500); 
  printChar("5");
  delay(500); 
  printChar("6");
  delay(500); 
  printChar("7");
  delay(500); 
  printChar("8");
  delay(500); 
  printChar("9");
  delay(500); 
  printChar("E");
  delay(500); 
  printChar("");
  */

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);
  Serial.println("setup() done connecting to ssid '" STASSID "'");

  timeClient.begin(); 
  timeClient.setTimeOffset(3600 * 2); //GMT +2
}

void loop() {

  printChar(""); 
  String payload = ""; 
  payload = getPayloadByUrl(urlOpenBugs); 

  if (payload != "")
  {
    if (isOpenBugsEmpty(payload))
    {
      //No open Bugs. Get last resolved Bug
      payload = getPayloadByUrl(urlDoneBugs); 

      if(payload != "")
      {
        printChar (String(getDaysWoBugs(payload))); 
      }else
      {
        //Error during second API-Call: Print "E"
        printChar("E"); 
      }
    }else
    {
      //There are open Bugs: Print "-"
      printChar("-"); 
    }
  }else
  {
    //Error during first API-Call: Print "E"
    printChar("E"); 
  }
  

  Serial.println("Wait 5m before next round...");
  delay(1000 * 60 * 5);
}



bool isOpenBugsEmpty(String payload)
{
  bool retVal = payload.length() < 30; 
  return retVal; 
}

int getDaysWoBugs(String payload)
{
  int retVal = 0; 

  //remove everything before resolutiondate
  payload = payload.substring(payload.lastIndexOf("resolutiondate") + 17); 

  //remove everything after resolutiondate
  payload.remove(19); 

  //convert to format yyyy-mm-dd hh:mm:ss
  payload.replace("T", " "); 

  Serial.println(payload); 


  //convert to datetime
  tmElements_t tm; 
  int year, month, day, hour, minute, second; 

  sscanf(payload.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
  tm.Year = CalendarYrToTm(year);
  tm.Month = month;
  tm.Day = day;
  tm.Hour = hour;
  tm.Minute = minute;
  tm.Second = second;

  unsigned long lastResolved =  makeTime(tm); 

  //Get current datetime
  timeClient.update(); 
  unsigned long currentTime = timeClient.getEpochTime(); 
  Serial.println("Times: "); 
  Serial.print("LastResolved: "); 
  Serial.println(lastResolved); 
  Serial.print("Current: "); 
  Serial.println(currentTime); 

  //Calculate date diff
  retVal = (currentTime - lastResolved); //Seconds
  Serial.print("ScondsWOBugs: "); 
  Serial.println(retVal); 
  retVal = retVal / 60; //Minutes
  Serial.print("MinutesWOBugs: "); 
  Serial.println(retVal);
  retVal = retVal / 60; //Hours
  Serial.print("HoursWOBugs: "); 
  Serial.println(retVal);
  retVal = retVal / 24; //Days
  Serial.print("DaysWOBugs: "); 
  Serial.println(retVal);
  
  return retVal; 
}

String getPayloadByUrl(String url)
{
  String payload = ""; 
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    //client->setFingerprint(fingerprint_sni_cloudflaressl_com);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    client->setInsecure();

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, url)) {  // HTTPS
      https.addHeader("Authorization", "Basic " + API_TOKEN ); 

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          payload = https.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }

  return payload; 

}

void printChar(String value)
{

  if(value == "0")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, HIGH);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, LOW);
    digitalWrite(LED_LL, HIGH);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "1")
  {
    digitalWrite(LED_T, LOW);
    digitalWrite(LED_TL, LOW);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, LOW);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, LOW);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "2")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, LOW);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, HIGH);
    digitalWrite(LED_LR, LOW);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "3")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, LOW);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "4")
  {
    digitalWrite(LED_T, LOW);
    digitalWrite(LED_TL, HIGH);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, LOW);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "5")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, HIGH);
    digitalWrite(LED_TR, LOW);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "6")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, HIGH);
    digitalWrite(LED_TR, LOW);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, HIGH);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "7")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, LOW);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, LOW);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, LOW);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "8")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, HIGH);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, HIGH);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "9")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, HIGH);
    digitalWrite(LED_TR, HIGH);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, HIGH);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "E")
  {
    digitalWrite(LED_T, HIGH);
    digitalWrite(LED_TL, HIGH);
    digitalWrite(LED_TR, LOW);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, HIGH);
    digitalWrite(LED_LR, LOW);
    digitalWrite(LED_L, HIGH);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "-")
  {
    digitalWrite(LED_T, LOW);
    digitalWrite(LED_TL, LOW);
    digitalWrite(LED_TR, LOW);
    digitalWrite(LED_M, HIGH);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, LOW);
    digitalWrite(LED_L, LOW);
    digitalWrite(LED_P, LOW);
  }
  else if(value == "")
  {
    digitalWrite(LED_T, LOW);
    digitalWrite(LED_TL, LOW);
    digitalWrite(LED_TR, LOW);
    digitalWrite(LED_M, LOW);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, LOW);
    digitalWrite(LED_L, LOW);
    digitalWrite(LED_P, LOW);
  }
  else 
  {
    digitalWrite(LED_T, LOW);
    digitalWrite(LED_TL, LOW);
    digitalWrite(LED_TR, LOW);
    digitalWrite(LED_M, LOW);
    digitalWrite(LED_LL, LOW);
    digitalWrite(LED_LR, LOW);
    digitalWrite(LED_L, LOW);
    digitalWrite(LED_P, HIGH);
  }

  Serial.println(); 
  Serial.println(value); 
}
