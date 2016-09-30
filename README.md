## what's it for?
The various sensors are interrogated regularly and send an alert by SMS if the critical threshold is reached.

## What components do I need? 
* an Arduino Uno or Nano or other (untested)
* a GSM module (from Adafruit : https://www.adafruit.com/product/1946 or other) 
* In most of case, a battery 3.7V 2500mAh wired to the GPRS module which need some power to connect to the provider
* gas sensor (in our case, MQ-2 and MQ-135)
* temperature and humidity probe as DHT-11( in the south of France we have mild temperatures). It can also work with DHT-22
* proof temperature probe DS18B20
* solar panel for complete power autonomy 

## What libraries are needed?
* Adafruit_FONA.h for GSM module
* DHT.h for temperature probe DHT
* OneWire.h for temperature probe DS18B20

## TODO
At this time, only alert are sent by SMS. The next step is to ** send data to web service ** through the GSM module. 
