
//*****************************************************************************************************
// Mereni teploty s ESP32 (ESP32-3 AI Thinker) a BME280 + DS18B20 + TMP117 (presna teplota) poslání na ThingSpeak
// R. Penizek
// 20.11.2022
//-----------------------------------------------------------------------------------------------------
// info: kod pro DS18B20 vychazi z kodu pro Spizirnu
//       ESP32 Board: Node32s
//       Additional board manager: https://dl.espressif.com/dl/package_esp32_index.json
//-----------------------------------------------------------------------------------------------------
// BlueDot_BME280:
// https://forum.arduino.cc/index.php?topic=593548.0
//
//TMP117 Adafruit
// knihovna: Adafruit TMP117
//-----------------------------------------------------------------------------------------------------
//ThinkgSpeak:
// https://github.com/mathworks/thingspeak-arduino
// ESP32 example: (je tam i priklad pro ESP8266)
//https://github.com/mathworks/thingspeak-arduino/tree/master/examples/ESP32/WriteMultipleFields
// Deep sleep:
//https://www.instructables.com/ESP32-Deep-Sleep-Tutorial/
//*****************************************************************************************************

#include "ThingSpeak.h"
#include "secrets.h"
#include <WiFi.h>
#include "BlueDot_BME280.h"
//Temperature Dallas DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
//Temperature TMP117
#include <Wire.h>
#include <Adafruit_TMP117.h>
#include <Adafruit_Sensor.h>

BlueDot_BME280 bme280 = BlueDot_BME280();

Adafruit_TMP117  tmp117;

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

// Initialize our values
float temperature = 15;
float pressure = 25;
//Wifi RSSI
int wifi_rssi;

// Parameters for DS18B20
float temperatureGround;
// GPIO where the DS18B20 is connected to
const int oneWireBus = 25;    //GPI25
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Parameters for TMP117
float temperatureTMP117;

void setup() {
  Serial.begin(115200);  //Initialize serial
  sensors.begin(); // Start the DS18B20 sensor

  // Try to initialize TMP117
if (!tmp117.begin()) {
  Serial.println("Failed to find TMP117 chip");
  while (1) { delay(10); }
}
Serial.println("TMP117 Found!");

  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {

  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
    
 // print the received signal strength:
  wifi_rssi =WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(wifi_rssi);
  }

//**************************
//BMP280 measurement
//**************************
Wire.begin(21, 22);  //sda scl

//*************ADVANCED SETUP
bme280.parameter.communication = 0;                  //Choose communication protocol
bme280.parameter.I2CAddress = 0x76;                  //Choose I2C Address
bme280.parameter.sensorMode = 0b11;                   //Choose sensor mode  //Choose sensor mode //0b00:     In sleep mode no measurements are performed, but power consumption is at a minimum //0b01:     In forced mode a single measured is performed and the device returns automatically to sleep mode //0b11:     In normal mode the sensor measures continually (default value)
bme280.parameter.IIRfilter = 0b100;                    //Setup for IIR Filter
bme280.parameter.tempOversampling = 0b101;             //Setup Temperature Ovesampling
bme280.parameter.tempOutsideCelsius = 15;              //default value of 15°C
bme280.parameter.pressureSeaLevel = 1013.25;
bme280.parameter.pressOversampling = 0b101;  
bme280.init();

//Measurement of temperature and pressure
temperature=bme280.readTempC();
//bme280.parameter.sensorMode = 0b00; // BMP280 sleep after measurement
pressure=bme280.readPressure();

//**************************
//DS18B20 measurement
//**************************
  //Temperature measurement of ground
  sensors.requestTemperatures(); 
  temperatureGround = sensors.getTempCByIndex(0);

//**************************
//TMP117 measurement
//**************************

sensors_event_t temp; // create an empty event to be filled
tmp117.getEvent(&temp); //fill the empty event object with the current measurements
temperatureTMP117=temp.temperature;

//**************************
//ThingSpeak data preparation
//**************************

// set the fields with the values
ThingSpeak.setField(1, temperature);
ThingSpeak.setField(2, temperatureGround);
ThingSpeak.setField(3, temperatureTMP117);
ThingSpeak.setField(4, pressure);
ThingSpeak.setField(5, wifi_rssi);

Serial.println("");
Serial.print("Temperature BME280:\t\t");
Serial.println(temperature);
Serial.print("Temperature DS18B20:\t\t");
Serial.println(temperatureGround);
Serial.print("Temperature TMP117:\t\t");
Serial.println(temperatureTMP117);
Serial.print(F("Pressure in [hPa]:\t\t")); 
Serial.println(pressure);
Serial.print("Wifi RSSI:\t\t");
Serial.println(wifi_rssi);

//**************************
//ThingSpeak send
//**************************

   // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

//delay
 delay(30000); // Wait 30 seconds to update the channel again
}
