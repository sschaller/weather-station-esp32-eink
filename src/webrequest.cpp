#include "webrequest.h"
#include "wifi_login.h"

#define NUM_TRIES 20

bool WebRequest::connect() {

  if (connected) {
    return true;
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  int num_tries = 0;
  while(WiFi.status() != WL_CONNECTED) {
    if (num_tries >= NUM_TRIES) {
      Serial.println("Could not connect to WiFi");
      return false;
    }
    num_tries++;
    
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  connected = true;
  
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

void WebRequest::disconnect() {
  if(!connected) {
    Serial.println("Not connected, no need to disconnect");
    return;
  }
  client.stop();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

WebRequest::~WebRequest() {
  if (connected) {
    disconnect();
  }
}