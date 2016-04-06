/* 
The MIT License (MIT)

Copyright (c)2016 David di Marcantonio

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/*
Version : 1.1
last update : 06 april 2016
*/

/* --------------
Include Libraries 
-----------------*/ 
/* -- Phone SIM Library ---- */
#include "Adafruit_FONA.h"
/* -- Temperature sensor -- */
#include "DHT.h"

/* -- Temperature probe DS18B20 --- */
#include "OneWire.h"

/* ------------------
Define pin connection
--------------------- */
#define FONA_RX 2             // Fona 
#define FONA_TX 3            // FOna 
#define FONA_RST 4           // FOna   
#define SENSOR_AMMONIA 0     // Ammonia / CO2 wired to analog pin 0
#define SENSOR_METHANE 1     // Methane sensor wired to analog pin 1 
#define DHTPIN 5             // DHT 11 wired to digital pin 5
#define DHTTYPE DHT11        // DHT 11
OneWire  ds(7);              // probe temp wired to digital pin 7

/* -------------------
initialize FONA object
-----------------------*/
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
uint8_t type;

/* ---------------------
Initialize DHT sensor.
---------------------- */
DHT dht(DHTPIN, DHTTYPE);

/* ------------------
Initialize varabiables 
---------------------- */
/* ------ sensor variables ------ */
int sensorValueAmmonia;
int sensorValueMethane;
int refAmmoniaValue = 350;
int refMethaneValue = 750;
int refCO2MinValue = 100;
float temperature;
float humidity;
float probeTemperature;

/* -------- fona variables --------- */
bool connectedToNetwork = false;
char PIN[] = "1234\0";                 // sim card PIN Code, must be initialize the first time,  be careful after 3 tries, the sim will be locked
//char sendtoInfo[] = "0626920632"; // david
char sendToDavid[] = "0626920632";
char sendToLea[] = "0643029517";
//char sendtoInfo[] = "0643029517"; // lea

//char sendToAlert[] = "0782693615"; // 2eme phone david
char sendToAlert[] = "0643029517"; // lea
char timeBuffer[23];
int contact = 1 ; // define who to contact

/* ---------- time variables ----------- */
int timeBeforeNextMethaneAlert = 0;
int timeBeforeNextAmmoniaAlert = 0; 
int timeBeforeNextCO2Alert = 0;
int timeToSend = 0;
int refTimeToSend = 600;           // define the time between each info sms (r * t = realtime) (be careful in depends of the delay between each trigger value)
int triggerDelay = 6000;         // define the time between each trigger values (impact arduino power consumption) each 1 min

/* #################
SETUP CODE, RUN ONCE 
##################### */
/* ------------------------------------------- SETUP -------------------------------------------------*/
void setup() 
{
  /* -----------------------
  start temperature sensor 
  ------------------------ */
  dht.begin();
  
  while (!Serial);
  /* ----------------
  Define serial bauds 
  ------------------- */
  Serial.begin(115200);
  Serial.println(F("FONA"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) 
  {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) 
  {
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
  if (imeiLen > 0) 
  {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
}

/* -------------------------------------------- LOOP (MAIN) ------------------------------------------*/
/* #####################
MAIN CODE, RUN REPATEDLY
######################## */
void loop() 
{
  
  /* ----------------
  test network status 
  -------------------- */
   // read the network/cellular status
   if (!connectedToNetwork) 
   {
        Serial.println("connected false ..., try to connect");
        uint8_t n = fona.getNetworkStatus();
        // ------------ The sim is not registered to any provider ------------------
        if (n == 0) 
        {
          Serial.println(F(" 0 - Not registered"));
          Serial.print(F("Unlocking SIM card: "));
          if (! fona.unlockSIM("1234\0")) 
          {
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
        // ------------- The sim is yet registered to a provider and cand send sms -------------
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
      
    /* --------------------
    Trigger analog values
    from sensor 
    ---------------------- */
  sensorValueAmmonia = analogRead(0);
  sensorValueMethane = analogRead(1);
  Serial.print("Ammonia : ");
  Serial.print(sensorValueAmmonia, DEC);
  Serial.print(" , Methane : ");
  Serial.println(sensorValueMethane, DEC);
  
  /* ----------------
  SEND ALERT BY SMS 
  ------------------ */
  /* ---- unexpected volume of ammonia gas ----- */
  if (sensorValueAmmonia > refAmmoniaValue) {
      if (timeBeforeNextAmmoniaAlert == 0) {
          sensAlertSMS(sendToLea, 2);
          timeBeforeNextAmmoniaAlert = 1000 * 60 * 60 * 2; // the next alert will be in 2 hours 
      }
    //sensAlertSMS(sendToAlert);
    
  }
  
  /* --------- maybe aerobic in process ----------- */
  if (sensorValueAmmonia < refCO2MinValue) {
      if (timeBeforeNextCO2Alert == 0) {
          sensAlertSMS(sendToLea, 2);
          timeBeforeNextCO2Alert = 1000 * 60 * 60 *2; // the next alert will be in 2 hours
      }
    
  }
  
  /* ------ unexpected volume of methane ---------- */
  if (sensorValueMethane > refMethaneValue) {
      if (timeBeforeNextMethaneAlert == 0) {
          sensAlertSMS(sendToLea, 1);
          timeBeforeNextMethaneAlert = 1000 * 60 * 60 * 2; // the next alert will be in 2 hours
      }
  }
  
  /* ----------------------------------
  Track temperature and pression values
 ------------------------------------- */
// Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  //float h = dht.readHumidity();
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  //float t = dht.readTemperature();
  temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  //float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  //float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" *C ");
  //Serial.print(f);
  //Serial.print(" *F\t");
  //Serial.print("Heat index: ");
  //Serial.print(hic);
  //Serial.print(" *C ");
  //Serial.print(hif);
  //Serial.println(" *F");
  
  /* --------------------------------
  track probe waterproof temperature 
  ----------------------------------- */
  probeTemperature = readTemperature();
  Serial.print("Probe temperature is : ");
  Serial.print(probeTemperature);
  Serial.print(" Deg.C , ");
  

/* ------------ 
define delay  
-------------- */  
  // add time to timeToSend;
  timeToSend++;
  Serial.print(" - delay1 - ");
  delay(triggerDelay); // trigger value each n milliseconds (could be less... for a better battery longevity)
  Serial.print(" - delay2 - ");
  // count down the alert timers 
  if (timeBeforeNextAmmoniaAlert > 0) timeBeforeNextAmmoniaAlert--;
  if (timeBeforeNextCO2Alert > 0) timeBeforeNextCO2Alert--;
  if (timeBeforeNextMethaneAlert > 0) timeBeforeNextMethaneAlert--;
  
 
  /* -------------
  SEND INFO BY SMS
  ---------------- */
  if (timeToSend == refTimeToSend) // each 30 secondes. The counter will be reinitialize to 0 in the sendSMS fn() 
  {
    Serial.println("Send sms");
    flushSerial();
    
    if ( contact == 1 ) {
      sendSMS(sendToDavid);
      contact = 2;
    } else {
      sendSMS(sendToLea);
      contact = 1;
    }
    
  }

} /* ------------------------------ END LOOP -------------------------------------- */

/* ----------------------- ADDITIONNAL FUNCTIONS --------------------------------- */
/* ----------------
Reinitialize serial
------------------ */
void flushSerial() 
{
  while (Serial.available())
    Serial.read();
}

/* ------
Send info SMS 
-------- */
void sendSMS(char contact[] ) 
{
  
  /* -----------------------------------------------
    String constructions (concatenations with values)
    -------------------------------------------------- */
    String message = "Infos : ";
    
    //BATTERY INFO 
    uint16_t vbat;
    message.concat("Batt. = ");
    if (! fona.getBattPercent(&vbat)) 
    {
          Serial.println(F("Failed to read Batt"));
    } else 
    {
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
    
    // TEMPERATURE AND HUMIDITY
    message.concat(" ,Temp : ");
    String temperatureString = String(temperature, 2);
    message.concat(temperatureString);
    message.concat(" ,Hum : ");
    String humidityString = String(humidity, 2);
    message.concat(humidityString);
    message.concat("%");
    
    // PROBE TEMPERATURE 
    message.concat(" ,Sonde : ");
    String probeString = String(probeTemperature, 2);
    message.concat(probeString);
    
    // CONVERT STRING TO CHAR ARRAY 
    int str_len = message.length() + 1; 
    char char_array[str_len];
    message.toCharArray(char_array, str_len);
    
    Serial.print(F("Send to #"));
    if (!fona.sendSMS(contact, char_array)) {
      Serial.println(F("Failed"));
    } else {
       Serial.println(F("Sent!"));
    }
    // reinitialize time counter 
    timeToSend = 0;
}

/* ----------------
Sendd Alert SMS 
-------------- */
void sensAlertSMS(char contact[], int type) 
{
  /* -----------------------------------------------
    String constructions (concatenations with values)
    -------------------------------------------------- */
    String message = "Alerte odeur : ";
    
    // alert Methane 
    if (type == 1 ) 
    {
      // METHANE  
      message.concat("CH4. : ");
      String methaneString = String(sensorValueMethane, DEC);
      message.concat(methaneString);
    }
    
    if (type == 2) 
    {
      //AMMONIA
      message.concat("NH3 : ");
      String AmmoniaString = String(sensorValueAmmonia, DEC);
      message.concat(AmmoniaString);
    }
    
    // CONVERT STRING TO CHAR ARRAY 
    int str_len = message.length() + 1; 
    char char_array[str_len];
    message.toCharArray(char_array, str_len);
    
    Serial.print(F("Send to #"));
    if (!fona.sendSMS(contact, char_array)) 
    {
      Serial.println(F("Failed"));
    } 
    else 
    {
       Serial.println(F("Sent!"));
    }
  
}

/* -------------------------
Read temperature from probe 
temperature waterproof sensor
---------------------------- */
float readTemperature() 
{
  byte data[12];
  byte addr[8];
  
  if (!ds.search(addr)) 
  {
    ds.reset_search();
    return -300; // there is no sensor wired 
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // tell sensor to start converting
  ds.reset();
  ds.select(addr);
  ds.write(0xBE); // tel sensor to start sending data
  for (int i = 0; i < 9; i++) 
  {
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];
  
  float raw = ((MSB << 8) | LSB); // move MSB left for 8 spaces , join that with LSB
  float realTempC = raw / 16; // move decimal point left for 4 spaces , result our temperature
  return realTempC;
}
    
