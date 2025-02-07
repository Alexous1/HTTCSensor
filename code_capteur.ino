
#include <Arduino.h>
#include "BaseFunction.h"
#include <Wire.h>
#define I2C_SDA 18
#define I2C_SCL 19



void setup() {

  // Initialise the ESP32 C3, wire, serial port and I2C bus
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(3, OUTPUT);
  pinMode(1, INPUT);
  digitalWrite(3, HIGH);
  Serial.begin(115200);

  //---------------------------------------------------------SPIFFS

  spif();


  //---------------------------------------------------------WIFI

  Wifi();

  //---------------------------------------------------------FOUND THE SENSOR AND RETRIEVES THE VALUES OF THE SENSOR

  rephresh();

  //--------------------------------------------------------- CONNECT TO THE MQTT BROKER AND SEND FEW MESSAGES WITH THE DATA TO GET THE VALUES OF THE SENSOR IN HOME ASSISTANT

  connectMqtt();
  sendValueMqtt();

  //--------------------------------------------------------- WE CAN HEDGED A WEBSITE ON THE ESP32 TO SET UP THE SENSORS

  WebSite();

}

void loop() {

  //--------------------------------------------------------- AFTER GET AND POST ALL OF THE INFORMATIONS ABOUT THE AIR QUALITY THE ESP32C3 FALL IN SLEEP
  deepSleep();

}
