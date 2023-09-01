#include <Wire.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h>
#include "Adafruit_SGP30.h"
#include "Adafruit_SHTC3.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "secret.h"


WiFiManager wm;                  //initialise le wifiManager
WiFiClient espClient;            // initialise le mqtt
PubSubClient client(espClient);  // initialise les fonction pour envoyer et recevoir des infos sur un broker mqtt


//fonction pour connaitre les valeurs de temperature et de l'humidite
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature));  // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                 // [mg/m^3]
  return absoluteHumidityScaled;
}


void spif(){

  // affiche tous les fichiers contenue dans le SPIFFS

  if (!SPIFFS.begin()) {
    Serial.println("Erreur SPIFFS...");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }

}

void deepSleep(){
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();

}

void rephresh() {

  #if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
      ;
    Serial.println();
  #endif

  //initialise les capteurs
  Adafruit_SGP30 sgp;
  Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

  // Verification du fonctionnement du SHTC3
  if (!shtc3.begin()) {
    while (1) delay(1);
  }

  // obtient les mesures des capteurs
  sensors_event_t humidity, temp;
  shtc3.getEvent(&humidity, &temp);

  //Verification du fonctionnement du SGP30
  if (!sgp.begin()) {
    while (1)
      ;
  }

  //verifie si les mesures du SGP30 sont reussies
  if (!sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }

  temperatur = temp.temperature;


  humidit = humidity.relative_humidity;
  Serial.println(humidit);
  CO2 = sgp.eCO2;
  tvoc = sgp.TVOC;
}


void getBatValue() {

  // transforme la valeur analogique en valeur explatable
  b = map(analogRead(BATTERYPIN), 0, 2640, 0, 120);
  Serial.println(b);


  // cherche la valeur de la batterie à l'aide du tableau vue tous en haut du script
  for (int x = 1; x <= 28; x++) {

    // regarde si la valeur de la batterie et entre deux éléments du tableaus
    if (minValueBat[x] <= b and b < maxValueBat[x]) {
      // attribut à la variable b la valeur de la batterie
      b = batterie[x];
      break;
    }
  }
}

void Wifi(){

    // initialise le wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    wifi += 1;
  
    // if (wifi >= 11){
    //   Serial.println("deep");
    //   deepSleep();
    // } 
  }
}

AsyncWebServer server(80);

void WebServer(){

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    int paramsNr = request->params();
    for (int i = 0; i < paramsNr; i++) {

      AsyncWebParameter *p = request->getParam(i);

      if (p->name() == "gender") {
        String inputMessage1;
        inputMessage1 = p->value();
        TIME_TO_SLEEP = inputMessage1.toInt() * 60;
        Serial.println(TIME_TO_SLEEP);
      }

      if (p->name() == "option") {
        String inputMessage1;
        inputMessage1 = p->value();
        reseau = inputMessage1.toInt();
        Serial.println(reseau);
      }

      if (p->name() == "mqttUser") {
        String inputMessage1;
        inputMessage1 = p->value();
        mqtt_username = inputMessage1.c_str();
        Serial.println(mqtt_username);
      }

      if (p->name() == "mqttIp") {
        String inputMessage1;
        inputMessage1 = p->value();
        mqtt_broker = inputMessage1.c_str();
        Serial.println(mqtt_broker);
      }

      if (p->name() == "mqttPassword") {
        String inputMessage1;
        inputMessage1 = p->value();
        mqtt_password = inputMessage1.c_str();
        Serial.println(mqtt_password);
      }

      if (p->name() == "mqttTopic") {
        String inputMessage1;
        inputMessage1 = p->value();
        topic = inputMessage1.c_str();
        Serial.println(topic);
      }

    }

    request->send(SPIFFS, "/main.html", "text/html");
  });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/w3.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    rephresh();
    
    String temp = String(temperatur);
    Serial.println(temp);
    request->send(200, "text/plain", temp);
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    rephresh();
    String hum = String(humidit);
    request->send(200, "text/plain", hum);
  });

  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request) {
    rephresh();
    String co2 = String(CO2);
    request->send(200, "text/plain", co2);
  });

  server.on("/tvoc", HTTP_GET, [](AsyncWebServerRequest *request) {
    rephresh();
    String tvo = String(tvoc);
    request->send(200, "text/plain", tvo);
  });

  server.on("/bat", HTTP_GET, [](AsyncWebServerRequest *request) {
    getBatValue();
    String bat = String(b);
    request->send(200, "text/plain", bat);
  });

  Serial.println("Serveur actif!");

  
}

void connectMqtt(){
  //connexion au serveur mqtt
  Serial.println("connect mqtt");
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("ESPClient", mqtt_username, mqtt_password)) {
      Serial.println("MQTT connecté");
    }
    else {
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
      delay(2000);
    }
  }
}



void sendValueMqtt(){

  Serial.println("send data");
  for (int z = 0; z <= 4; z++) {
    DynamicJsonDocument doc(2048);
    doc["device_class"] =  device[z];
    doc["name"] = name[z];
    doc["state_topic"] = state_topic;
    doc["unique_id"] = unique_id[z];
    doc["unit_of_measurement"] = unit_of_measurement[z];
    doc["value_template"] = value_template[z];

    char buf[256];
    serializeJson(doc, buf);
    client.publish(top[z], buf, true);
    Serial.println(device[z]);
    delay(1000);
  }

  getBatValue();
  rephresh();

  //publie les valeurs pour homeassistant
  DynamicJsonDocument doc(2048);
  doc["hum"] = humidit;
  doc["temp"] = temperatur;
  doc["tvoc"] = tvoc;
  doc["eco2"] = CO2;
  doc["batterie"] = b;
  doc["retainFlag"] = true;

  String message = "";
  serializeJson(doc, message);
  Serial.println("debuf");

  client.publish("esp32/test/", (char*) message.c_str(), true);
  delay(2000);

}

void WebSite(){

  if (digitalRead(19) == HIGH) {

    if (reseau == 1) {
      //creer l'AP
      WiFi.softAP(ssid, password);
      Serial.println(WiFi.softAPIP());
    }

    Serial.println(WiFi.localIP());
    WebServer();
    server.begin();
    Serial.println("tranquille");
  }
}