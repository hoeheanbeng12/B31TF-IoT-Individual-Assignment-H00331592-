#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:hwu.esp32:H00331592" 
#define MQTT_USER ""
#define MQTT_TOKEN ""
#define MQTT_TOPIC "d:hwu.esp32:H00331592/evt/status/fmt/json"
#define MQTT_TOPIC_DISPLAY "d:hwu.esp32:H00331592/cmd/display/fmt/json"
#define MQTT_TOPIC_COMMAND "d:hwu.esp32:H00331592/cmd/#"

#define RGB_PIN 5
#define DHT_PIN 4

#define DHTTYPE DHT11
#define NEOPIXEL_TYPE NEO_GRB + NEO_KHZ800

#define ALARM_COLD 13.0
#define ALARM_HOT 30.0
#define WARN_COLD 18.0
#define WARN_HOT 25.0


char ssid[] = "VM8449490 (2.4GHz)";
char pass[] = "Rb4dkdrhvDkv";  //


Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, RGB_PIN, NEOPIXEL_TYPE);
DHT dht(DHT_PIN, DHTTYPE);


void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);


StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
static char msg[50];

float h = 0.0;
float t = 0.0;

unsigned char r = 0;
unsigned char g = 0;
unsigned char b = 0;

long reporting_interval = 10000;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");

  String message = "";
  for(int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  if (strstr(topic, "cmd/interval")) {  
    StaticJsonDocument<100> cmdDoc;
    DeserializationError error = deserializeJson(cmdDoc, message);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    if (cmdDoc.containsKey("Interval")) {
      int seconds = cmdDoc["Interval"];
      reporting_interval = (long)seconds * 1000; 
      Serial.print("Reporting interval updated to: ");
      Serial.print(reporting_interval);
      Serial.println(" ms");
    }
  } 
  
  else if (strstr(topic, "cmd/display")) {
    
    StaticJsonDocument<100> cmdDoc;
    DeserializationError error = deserializeJson(cmdDoc, message);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }


    if (cmdDoc.containsKey("r") && cmdDoc.containsKey("g") && cmdDoc.containsKey("b")) {
      unsigned char r_cmd = cmdDoc["r"];
      unsigned char g_cmd = cmdDoc["g"];
      unsigned char b_cmd = cmdDoc["b"];
      
      Serial.println("Setting LED color from cloud command.");
      pixel.setPixelColor(0, r_cmd, g_cmd, b_cmd);
      pixel.show();
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }
  Serial.println();
  Serial.println("ESP32C3 Sensor Application");


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");


  dht.begin();
  pixel.begin();


  if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
    Serial.println("MQTT Connected");
    mqtt.subscribe(MQTT_TOPIC_COMMAND);

  } else {
    Serial.println("MQTT Failed to connect!");
    ESP.restart();
  }
}

void loop() {
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected");
      mqtt.subscribe(MQTT_TOPIC_COMMAND);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }
 
  h = dht.readHumidity();
  t = dht.readTemperature(); // uncomment this line for centigrade
  // t = dht.readTemperature(true); // uncomment this line for Fahrenheit


    status["temp"] = t;
    status["humidity"] = h;
    serializeJson(jsonDoc, msg, 50);
    Serial.println(msg);
    if (!mqtt.publish(MQTT_TOPIC, msg)) { 
      Serial.println("MQTT Publish failed");
    }
//  }

  int intervals = reporting_interval/1000;

  for (int i = 0; i < intervals; i++) {
    Serial.print("Reporting Interval: ");
    Serial.println(intervals);
    
    mqtt.loop();
    delay(1000);
  }
}
