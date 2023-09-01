
#include <Arduino.h>
#include "BaseFunction.h"




void setup() {

  // initialisation du port série
  Serial.begin(115200);
  // initialise la broche d'alimentationa
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  // initialise la broche battery
  pinMode(35, INPUT);
  // initialise la broche du web buton
  pinMode(19, INPUT);

  //---------------------------------------------------------SPIFFS


  spif();


  //---------------------------------------------------------WIFI

  Wifi();


  //---------------------------------------------------------FOUND THE SENSOR


  rephresh();


  //---------------------------------------------------------BATTERY VALUE


  getBatValue();


  //--------------------------------------------------------- MQTT

  connectMqtt();
  sendValueMqtt();


  WebSite();

}

void loop() {

  deepSleep();

}