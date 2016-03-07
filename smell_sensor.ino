/* 
The MIT License (MIT)

Copyright (c) 2016 David di Marcantonio

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/* --------------
Include Libraries 
-----------------*/ 
#include "Adafruit_FONA.h"


/* ------------------
Define pin connection
--------------------- */
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define SENSOR_AMMONIA 0
#define SENSOR_METHANE 1

/* -------------------
initialize FONA object
-----------------------*/
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
uint8_t type;

/* ------------------
Initialize varabiables 
---------------------- */
/* ------ sensor variables ------ */
int sensorValueAmmonia;
int sensorValueMethane;
/* -------- fona variables --------- */
bool connectedToNetwork = false;
char PIN[5] = "1243\0";
char sendto[] = "0626920632";
char timeBuffer[23];
int timeToSend = 0;

/* #################
SETUP CODE, RUN ONCE 
##################### */
/* ------------------------------------------- SETUP -------------------------------------------------*/
void setup() {
  while (!Serial);
  /* ----------------
  Define serial bauds 
  ------------------- */
  Serial.begin(115200);
  Serial.println(F("FONA"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case FONA800L:
      Serial.println(F("FONA 800L")); break;
    case FONA800H:
      Serial.println(F("FONA 800H")); break;
    case FONA808_V1:
      Serial.println(F("FONA 808 (v1)")); break;
    case FONA808_V2:
      Serial.println(F("FONA 808 (v2)")); break;
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default: 
      Serial.println(F("???")); break;
  }
  
  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
  
}


/* #####################
MAIN CODE, RUN REPATEDLY
######################## */
/* -------------------------------------------- LOOP (MAIN) ------------------------------------------*/
void loop() {
  /* ----------------
  test network status 
  -------------------- */
   // read the network/cellular status
   if (!connectedToNetwork) {
       Serial.println("connected false ..., try to connect");
        uint8_t n = fona.getNetworkStatus();
        if (n == 0) {
          Serial.println(F(" 0 - Not registered"));
          Serial.print(F("Unlocking SIM card: "));
          if (! fona.unlockSIM("6696\0")) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
            connectedToNetwork = true;
          }
        }
        if (n == 1) Serial.println(F("1 - Registered (home)"));
        if (n == 2) Serial.println(F("2 - Not registered (searching)"));
        if (n == 3) Serial.println(F("3 - Denied"));
        if (n == 4) Serial.println(F("4 - Unknown"));
        if (n == 5) {
          Serial.println(F("5 - Registered roaming"));
          connectedToNetwork = true;
          if (!fona.enableNetworkTimeSync(true)) {
            Serial.println(F("Failed to enable"));
          } else {
            fona.getTime(timeBuffer, 23);  // make sure replybuffer is at least 23 bytes!
            Serial.print(F("Time = ")); Serial.println(timeBuffer);
          }  
        }
      }

  // put your main code here, to run repeatedly:
  sensorValueAmmonia = analogRead(0);
  sensorValueMethane = analogRead(1);
  Serial.print("Ammonia : ");
  Serial.print(sensorValueAmmonia, DEC);
  Serial.print(" , Methane : ");
  Serial.println(sensorValueMethane, DEC);
  // add time to timeToSend;
  timeToSend++;
  delay(1000);
  
  /* -------------
  SEND SMS 
  ---------------- */
  if (timeToSend == 30) {
    Serial.println("Send sms");
    flushSerial();
    
    /* -----------------------------------------------
    String constructions (concatenations with values)
    -------------------------------------------------- */
    String message = "Infos : ";
    
    //BATTERY INFO 
    uint16_t vbat;
    message.concat("Batt. = ");
    if (! fona.getBattPercent(&vbat)) {
          Serial.println(F("Failed to read Batt"));
      } else {
        String batString = String(vbat, DEC);
        Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
        message.concat(batString);
        message.concat("% ,");
      }
    
    // METHANE  
    message.concat("CH4. : ");
    String methaneString = String(sensorValueMethane, DEC);
    message.concat(methaneString);
    
    //AMMONIA
    message.concat(" ,NH3 : ");
    String AmmoniaString = String(sensorValueAmmonia, DEC);
    message.concat(AmmoniaString);
    
    // CONVERT STRING TO CHAR ARRAY 
    int str_len = message.length() + 1; 
    char char_array[str_len];
    message.toCharArray(char_array, str_len);
    
    Serial.print(F("Send to #"));
    if (!fona.sendSMS(sendto, char_array)) {
      Serial.println(F("Failed"));
    } else {
       Serial.println(F("Sent!"));
    }
    // reinitialize time counter 
    timeToSend = 0;
  }

} /* ----------------- END LOOP ----------------- */



/* ----------------
Reinitialize serial
------------------ */
void flushSerial() {
  while (Serial.available())
    Serial.read();
}
