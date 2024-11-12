#include <MQ135.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "DHT.h"          //Les différentes bibliothèques utilisé

#define PIN_MQ135 A0
#define DHTPIN 4

#define DHTTYPE DHT22

MQ135 mq135_sensor(PIN_MQ135);

DHT dht(DHTPIN, DHTTYPE);

//création des variables//////

// WiFi settings
const char *ssid = "<SSID du Wi-Fi>";
const char *password = "<Mot de passe Wi-Fi>";

// MQTT Broker settings
const char *mqtt_broker = "<Adresse IP du broker MQTT>";// broker endpoint
const char *mqtt_topic = "/hopital";
const char *mqtt_topic_gas = "/hopital/gas";// MQTT topic
const char *mqtt_topic_temp = "/hopital/temp";
const char *mqtt_topic_humi = "/hopital/humi";
const char *mqtt_username = "ESP8266";// Nom d'utilisateur pour accéder au broker
const char *mqtt_password = "capteur";// Mot de passe pour accéder au broker
const int mqtt_port = 1883;// MQTT port (TCP)

///////////////////////////////

float temperature = 21.0; // Assume current temperature. Recommended to measure with DHT22
float humidity = 25.0; // Assume current humidity. Recommended to measure with DHT22

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void connectToWiFi();

void connectToMQTTBroker();

void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup() {
    Serial.begin(115200);
    Serial.println(F("DHTxx test!"));
    dht.begin();
    connectToWiFi();
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTTBroker();
}

void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) { //tant que l'ESP n'est pas connecté au Wi-Fi, ajouter des "..." dans le moniteur série
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to the WiFi network");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^"); //Quand l'esp se connecte au au broker MQTT, il envoie ce message vers le topic principal
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void loop() {
    char message[50];
    if (!mqtt_client.connected()) {
        connectToMQTTBroker();
    }
    mqtt_client.loop();
    float rzero = mq135_sensor.getRZero();
    float correctedRZero = mq135_sensor.getCorrectedRZero(temperature, humidity);
    float resistance = mq135_sensor.getResistance();
    float ppm = mq135_sensor.getPPM()/100; //La valeur est divisée par 100 car la valeur d'origine était beaucoup trop grosse
    float correctedPPM = mq135_sensor.getCorrectedPPM(temperature, humidity)/100;
    Serial.print(correctedPPM);
    dtostrf(correctedPPM, 4, 2,message);
    mqtt_client.publish(mqtt_topic_gas, message); //envoie des données de gaz vers le topic /hopital/gas
    delay(2000);

    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
    }
    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    //Serial.print(F("Humidity: "));
    Serial.print(h);
    //Serial.print(F("%  Temperature: "));
    //Serial.print(t);
    //Serial.print(F("°C "));
    //Serial.print(f);
    //Serial.print(F("°F  Heat index: "));
    Serial.print(hic);
    //Serial.print(F("°C "));
    //Serial.print(hif);
    //Serial.println(F("°F"));
    dtostrf(hic, 4, 2,message);
    //mqtt_client.publish(mqtt_topic_temp, "Temperature (°C):");
    mqtt_client.publish(mqtt_topic_temp, message); //envoie des données de température vers le topic /hopital/temp
    dtostrf(h, 4, 2,message);
    //mqtt_client.publish(mqtt_topic_temp, "Humidité (%):");
    mqtt_client.publish(mqtt_topic_humi, message); //envoie des données d'humidité vers le topic /hopital/humi
    delay(300);
}
