#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "wifi_login.h"

WiFiClientSecure client;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  delay(100);

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, 443)) {
    Serial.println("Connection failed!");
    return;
  }
  
  Serial.println("Connected to server!");
  // Make a HTTP request:
  client.println(headers);
  client.println();

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  
  DynamicJsonDocument doc(12000);

  // Deserialize the JSON document    
  DeserializationError error = deserializeJson(doc, client);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  JsonArray forecast = doc["forecast"]["forecast"];
  int start = doc["graph"]["start"];
  JsonArray temperatureMin1h = doc["graph"]["temperatureMin1h"];
  JsonArray temperatureMax1h = doc["graph"]["temperatureMax1h"];
  JsonArray temperatureMean1h = doc["graph"]["temperatureMean1h"];
  JsonArray precipitationMean1h = doc["graph"]["precipitationMean1h"];
  
  for(JsonObject f : forecast) {
    const char *date = f["dayDate"].as<char *>();
    int icon = f["iconDay"].as<int>();
    int temperatureMax = f["temperatureMax"].as<int>();
    int temperatureMin = f["temperatureMin"].as<int>();
    int precipitation = f["precipitation"].as<int>();

    Serial.print(date);
    Serial.print(" ");
    Serial.print(icon);
    Serial.print(" ");
    Serial.print(temperatureMax);
    Serial.print(" ");
    Serial.print(temperatureMin);
    Serial.print(" ");
    Serial.print(precipitation);
    Serial.println("");
  }

  client.stop();
}

void loop() {

}