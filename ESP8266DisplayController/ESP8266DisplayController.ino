#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266MQTTClient.h>
#include <ArduinoJson.h>

MQTTClient mqtt;
bool firstMessage = true;
void setup() {
  Serial.begin(115200);
  Serial.println("hello");
  WiFiManager wifiManager;

  wifiManager.autoConnect("ESP8266 Display Controller");

  //topic, data, data is continuing
  mqtt.onData([](String topic, String data, bool cont) {
    Serial.printf("Data received, topic: %s, data: %s\r\n", topic.c_str(), data.c_str());
    mqtt.unSubscribe("/qos0");
  });

  mqtt.onSubscribe([](int sub_id) {
    Serial.printf("Subscribe topic id: %d ok\r\n", sub_id);
    mqtt.publish("/qos0", "qos0", 0, 0);
  });
  mqtt.onConnect([]() {
    Serial.printf("MQTT: Connected\r\n");
    Serial.printf("Subscribe id: %d\r\n", mqtt.subscribe("/qos0", 0));
//    mqtt.subscribe("/qos1", 1);
//    mqtt.subscribe("/qos2", 2);
  });
  mqtt.onDisconnect([]() {
    Serial.printf("MQTT: Disconnected\r\n");
  });
}

void parseObjects(String serialdata) {
  Serial.println(serialdata);
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, serialdata);
  if (err) {
    Serial.println("ERR: "+String(err.c_str()));
    return;
  }
  
  JsonVariant ip = doc["ip"];
  JsonVariant id = doc["id"];
  JsonVariant g = doc["g"];
  JsonVariant req = doc["req"];
  if (ip.isNull()||id.isNull()||g.isNull()||req.isNull()) {
    Serial.println("ERR: JSON INCOMPLETE");
    return;
  }
  Serial.println(ESP.getFreeHeap());
  if (WiFi.status() == WL_CONNECTED) {
    if(firstMessage){
      char url[128];
      sprintf(url ,"mqtt://%s:1883", ip.as<char*>());
      mqtt.begin(url);
      firstMessage = false;
    }
    
    char topic[128];
    sprintf(topic, "lights/%i/rgb_cct/%i", id.as<int>(), g.as<int>());
      
    char body_buf[256];
    serializeJson(req, body_buf);
    
    mqtt.publish(topic, body_buf);
  } else {
    Serial.println("ERR: WIFI");
  }

}

void loop() {
  if (Serial.available() > 0) {
    parseObjects(Serial.readStringUntil('\n'));
  }
  if(!firstMessage){
    mqtt.handle();
  }
  yield();
}
