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


WiFiManager wm;                  // Initializes the wifi manager
WiFiClient espClient;            // Initializes the MQTT client
PubSubClient client(espClient);  // Initializes the MQTT client

AsyncWebServer server(80);  // Initializes the website


void spif() {

  // on esp32C3 we can stock few files in the memory of the esp32
  // with this lines we try to get the files and to read them

  // if we can't read the files, we post on the serial port a error message
  if (!SPIFFS.begin()) {
    Serial.println("Erreur SPIFFS...");
    return;
  }

  // but if we can read the file we open them and get the information in the files
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  // for each files we print in the serial port the name of the files
  while (file) {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }
}

void getBatValue() {

  // with this line, we read the voltage of the pin
  int rawValue = analogRead(1);

  // then we convert the voltage to 12 bits
  Vout = (rawValue / 4095.0) * 3.3;
  Serial.println(Vout);

  // We calcul the voltage of the battery
  Vin = Vout * (1000 + 3300) / 3300;

  // Afficher les résultats
  Serial.print("Vout (ADC) : ");
  Serial.print(Vin);


  // we search in a table the value of the battery
  for (int x = 1; x <= 28; x++) {

    // see if the value of the battery is between the two values of the table
    if (minValueBat[x] <= b and b < maxValueBat[x]) {
      // set up the voltage of the battery in a variable
      b = batterie[x];
      Serial.println(b);
      break;
    }
  }
}

void deepSleep() {
  // for full sp32 we can put them in deep sleep their consumption is then very low like 10 microampere and it wakes up all alone after a while
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}


void rephresh() {

  // initialize the sensors
  Adafruit_SGP30 sgp;
  Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
  Wire.begin(18, 19);

  Wire.beginTransmission(0x70);
  Wire.write(0x35);
  Wire.write(0x17);
  Wire.endTransmission();

  while (!Serial)
    delay(10);

  // try to find the shtc3
  Serial.println("SHTC3 test");
  if (!shtc3.begin()) {
    Serial.println("Couldn't find SHTC3");
    while (1) delay(1);
  }
  Serial.println("Found SHTC3 sensor");

  // try to find the shp30
  Serial.println("test");
  #if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
      ;
    Serial.println("eie");
  #endif

  // get the values of the shtc3
  sensors_event_t humidity, temp;
  shtc3.getEvent(&humidity, &temp);

  //check if the sgp30 work correctly
  if (!sgp.begin()) {
    while (1)
      ;
  }

  //verify if the mesure of the sgp30 are done
  if (!sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }

  // assign for each variable the value of the sensor
  temperatur = temp.temperature;
  Serial.println(temperatur);


  humidit = humidity.relative_humidity;
  Serial.println(humidit);
  CO2 = sgp.eCO2;
  Serial.println(CO2);
  tvoc = sgp.TVOC;
}

void Wifi() {

  // initialize the wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    wifi += 1;
    Serial.print(".");
    delay(100);

    // if not find the wifi, put the esp32 in deep sleep
    if (wifi >= 91) {
      deepSleep();
    }
  }
  // print the IP adress of the esp32
  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}



void WebServer() {

  // opens the files (html and css files) for the website
  if (!SPIFFS.begin(true)) {  // true pour formater si nécessaire
    Serial.println("Erreur : SPIFFS non monté !");
    return;
  }
  Serial.println("SPIFFS monté avec succès.");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // int paramsNr = request->params();
    // for (int i = 0; i < paramsNr; i++) {

    //   AsyncWebParameter *p = request->getParam(i);

    //   if (p->name() == "gender") {
    //     String inputMessage1;
    //     inputMessage1 = p->value();
    //     TIME_TO_SLEEP = inputMessage1.toInt() * 60;
    //     Serial.println(TIME_TO_SLEEP);
    //   }

    //   if (p->name() == "option") {
    //     String inputMessage1;
    //     inputMessage1 = p->value();
    //     reseau = inputMessage1.toInt();
    //     Serial.println(reseau);
    //   }

    //   if (p->name() == "mqttUser") {
    //     String inputMessage1;
    //     inputMessage1 = p->value();
    //     mqtt_username = inputMessage1.c_str();
    //     Serial.println(mqtt_username);
    //   }

    //   if (p->name() == "mqttIp") {
    //     String inputMessage1;
    //     inputMessage1 = p->value();
    //     mqtt_broker = inputMessage1.c_str();
    //     Serial.println(mqtt_broker);
    //   }

    //   if (p->name() == "mqttPassword") {
    //     String inputMessage1;
    //     inputMessage1 = p->value();
    //     mqtt_password = inputMessage1.c_str();
    //     Serial.println(mqtt_password);
    //   }

    //   if (p->name() == "mqttTopic") {
    //     String inputMessage1;
    //     inputMessage1 = p->value();
    //     topic = inputMessage1.c_str();
    //     Serial.println(topic);
    //   }

    // }

    request->send(SPIFFS, "/main1.html", "text/html");
  });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/w3.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  // replace a number by the value of the temperature
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    String temp = String(temperatur);
    request->send(200, "text/plain", temp);
  });

  // replace a number by the value of the humidity
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    String hum = String(humidit);
    request->send(200, "text/plain", hum);
  });

  // replace a number by the value of the co2
  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request) {
    String co2 = String(CO2);
    request->send(200, "text/plain", co2);
  });

  // replace a number by the value of the tvoc
  server.on("/tvoc", HTTP_GET, [](AsyncWebServerRequest *request) {
    String tvo = String(tvoc);
    request->send(200, "text/plain", tvo);
  });

  // server.on("/bat", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   getBatValue();
  //   String bat = String(b);
  //   request->send(200, "text/plain", bat);
  // });
}

void connectMqtt() {
  //connection to the MQTT broker
  Serial.println("connect mqtt");
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("ESPClient", mqtt_username, mqtt_password)) {
      Serial.println("MQTT connecté");
    } else {
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
      delay(2000);
    }
  }
}


void sendValueMqtt() {

  // send on the MQTT broker a parameters message for each parameter of the area quality
  Serial.println("send data");
  for (int z = 0; z <= 4; z++) {
    DynamicJsonDocument doc(2048);
    doc["device_class"] = device[z];
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

  // calls the function to reconnect to mqtt broker and retrieve sensor values
  connectMqtt();
  rephresh();

  //publish to the MQTT broker the values of the different sensor
  DynamicJsonDocument doc1(2048);
  doc1["hum"] = humidit;
  doc1["temp"] = temperatur;
  doc1["tvoc"] = tvoc;
  doc1["eco2"] = CO2;
  doc1["batt"] = b;
  doc1["retainFlag"] = true;

  String message = "";
  serializeJson(doc1, message);
  Serial.println("debuf");

  client.publish("esp32C3/test/", (char *)message.c_str(), true);
  delay(2000);
}

void WebSite() {

  // print the IP adress of the esp32 and create the website
  Serial.println(WiFi.localIP());
  WebServer();

  // if the reseau is equal to 1, create an access point
  if (reseau == 1) {
    WiFi.softAP(ssid, password);
    Serial.println(WiFi.softAPIP());
  }

  // if we are connected to the wifi, start the server
  if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0) {
    server.begin();

  } else {
    Serial.println("WiFi non prêt, réessayez.");
  }
}
