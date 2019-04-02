#include <NTPClient.h>
#include <WiFiUdp.h>

#include "webrequest.h"
#include "wifi_login.h"

bool WebRequest::connect() {

  if (connected) {
    return true;
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int num_tries = 0;
  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED && num_tries < 5) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
    
    if (num_tries >= 5) {
      Serial.println("Could not connect to WiFi");
      return false;
    }
    num_tries++;
  }

  Serial.print("Connected to ");
  Serial.println(ssid);
  
  return true;
}

bool WebRequest::requestWeather() {
  if (!connect()) {
    return false;
  }

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, 443)) {
    Serial.println("Connection failed!");
    return false;
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
  
  return true;
}

bool WebRequest::updateTime() {
  if (!connect()) {
    return false;
  }
  
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);

  timeClient.begin();
  bool success = timeClient.forceUpdate();
  timeClient.end();
  
  return success;
}

void WebRequest::disconnect() {
  if(!connected) {
    Serial.println("Not connected, no need to disconnect");
    return;
  }
  client.stop();
}

WebRequest::~WebRequest() {
  if (connected) {
    disconnect();
  }
}