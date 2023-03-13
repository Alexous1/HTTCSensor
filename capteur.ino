// import des librairy
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Adafruit_SGP30.h"
#include "Adafruit_SHTC3.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#else
#endif
#include <ESP_Mail_Client.h>

//definition des variables du wifi
#define WIFI_SSID "********"
#define WIFI_PASSWORD "********"

//definition des variables de gmail
#define SMTP_HOST "********"
#define SMTP_PORT esp_mail_smtp_port_587
#define AUTHOR_EMAIL "********"
#define AUTHOR_PASSWORD "********"

//definitons des variables pour le DeepSleep
RTC_DATA_ATTR int bootCount = 0;
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  900

//definitions des variables pour le broker mqtt
const char *mqtt_broker = "********";
const char *topic = "********";
const char *mqtt_username = "********";
const char *mqtt_password = "********";
const int mqtt_port = 1883;

//definitions des variables quelquonque
const int BATTERYPIN = 35; //pin de la batterie
int b = 0;
int valeur = 0;
bool ok = false;

//creations du tableau
int batterie[] = {3, 6, 9, 13, 17, 20, 24, 26, 32, 35, 40, 43, 47, 51, 55, 58, 62, 65, 69, 73, 77, 81, 85, 89, 92, 96, 100};
int minValueBat[] = {0, 10, 20, 27, 35, 37, 40, 44, 48, 51, 55, 58, 62, 66, 70, 72, 75, 77, 80, 83, 86, 89, 92, 96, 100, 110, 120};
int maxValueBat[] = {10, 20, 27, 35, 37, 40, 44, 48, 51, 55, 58, 62, 66, 70, 72, 75, 77, 80, 83, 86, 89, 92, 96, 100, 110, 120};

WiFiClient espClient;
PubSubClient client(espClient);
SMTPSession smtp;
void smtpCallback(SMTP_Status status);

void setup() {
  Serial.begin(115200);
  Serial.println("coucouc");
  pinMode(36, OUTPUT);
  pinMode(23, OUTPUT);
  pinMode(35, INPUT);
  digitalWrite(23, HIGH);
  digitalWrite(36, HIGH);

// connexion au wifi
  WiFi.mode(WIFI_STA); //Optional
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
  }
// ecrit dans le moniteur serie l'adresse ip de la carte
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

#if defined(ARDUINO_ARCH_SAMD)
  while (!Serial)
    ;
  Serial.println();

#endif

  smtp.debug(1);
  smtp.callback(smtpCallback);
  Adafruit_SGP30 sgp;
  Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

  // Verification du fonctionnement du SHTC3
  if (! shtc3.begin()) {
    while (1) delay(1);
  }

  //Verification du fonctionnement du SGP30
  if (! sgp.begin()) {
    while (1);
  }
  
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  sensors_event_t humidity, temp;
  shtc3.getEvent(&humidity, &temp);

  //ecrit dans le moniteur serie les valeurs de la temperature et de l'humidité
  Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
  delay(100);

  //verifie si les mesures du SGP30 sont reussies
  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }

  //ecrit dans le moniteur serie les valeurs du co2 et du TVOC (Taux de Volatiles Composé dans l'Air 
  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");
  delay(100);

  //connexion au serveur
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    if (client.connect(client_id.c_str())) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      delay(2000);
      Serial.println("deepSleep");
      esp_deep_sleep_start();
    }
  }



   //code pour trouver le pourcentage de batterie
  b = analogRead(BATTERYPIN); //valeur analogique
  b = map(b, 0, 2640, 0, 120);
  Serial.println(b);
  for (int x = 1; x <= 28; x++) {
    Serial.println(x);
    if (minValueBat[x] <= b  and b < maxValueBat[x]){
      Serial.println(b);
      b = batterie[x];
      Serial.println(b);
      break;
    }
    else{
      Serial.println("pass");
    }
  }

  if(b==100){
    batterieCharge("batterie chargée à 100 %");
  }

  else if(b==9){
    batterieCharge("batterie faible, pensez à charger la batterie");
  }

  //publie les valeurs sur le broker
  DynamicJsonDocument dooc1(2048);
  dooc1["device_class"] =  "temperature";
  dooc1["name"] = "th_chambre_alexis_temperature";
  dooc1["state_class"] = "measurement";
  dooc1["state_topic"] = "esp8266/test";
  dooc1["unit_of_measurement"] = "°C";
  dooc1["value_template"] = "{{ value_json.temp }}";

  char buffer[256];
  serializeJson(dooc1, buffer);
  client.publish("homeassistant/sensor/esp32/temperature/config", buffer, true);


  //publie les paramètre du message pour homeassistant
  DynamicJsonDocument doc(2048);
  doc["temp"] = temp.temperature;
  doc["hum"] = humidity.relative_humidity;
  doc["tvoc"] = sgp.TVOC;
  doc["eco2"] = sgp.eCO2;
  doc["batterie"] = b;
  doc["retainFlag"] = true;

  String message = "";
  serializeJson(doc, message);
  Serial.println("debuf");

  client.publish("esp8266/test/", (char*) message.c_str(), true);
  delay(3000);

  //Configuration du timer pour le deepSleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  delay(100);
}

void loop() {
  esp_deep_sleep_start();
}

//fonction pour connaitre les valeurs de temperature et de l'humidite
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
  return absoluteHumidityScaled;
}


void mqtt_publish(String topic, float t) {
  char top[topic.length() + 1];
  topic.toCharArray(top, topic.length() + 1);
  char t_char[550];
  String t_str = String(t);
  t_str.toCharArray(t_char, t_str.length() + 1);
  client.publish(top, t_char);
}

void batterieCharge(String phrase) {

  ESP_Mail_Session session; 

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = F("mydomain.net");

  session.time.ntp_server = F("********");
  session.time.gmt_offset = 3;
  session.time.day_light_offset = 0;

  SMTP_Message message;

  message.sender.name = F("ESP capteur 2000");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("danger");
  message.addRecipient("un nom ici", "********");

  String htmlMsg = phrase;
  message.html.content = htmlMsg;

  message.html.charSet = F("us-ascii");
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.addHeader(F("Message-ID: ********"));

  if (!smtp.connect(&session))
    return;

  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

  ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

//fonction qui affiche toute les info sur le message qui est publier sur le broker
void smtpCallback(SMTP_Status status)
{

  Serial.println(status.info());

  if (status.success())
  {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");
    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", asctime(localtime(&ts)));
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
    smtp.sendingResult.clear();
  }
}
