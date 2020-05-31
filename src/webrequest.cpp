#include "webrequest.h"
#include "wifi_login.h"

#define NUM_TRIES 4
#define WAIT_TIME 5

bool WebRequest::connect() {

  if (connected) {
    return true;
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  
  for(int num_tries = 0; num_tries < NUM_TRIES * WAIT_TIME; num_tries++) {
    
    if (num_tries % WAIT_TIME == 0) {
      WiFi.begin(ssid, password);
    }

    Serial.print(".");
    delay(1000);

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected to ");
      Serial.println(ssid);

      connected = true;
      return true;
    }
  }
  
  Serial.println("Could not connect to WiFi");
  return false;
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

  Serial.println("Sent request!");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    // Serial.println(line);
    if (line == "\r") {
      Serial.println("headers received");
      return true;
    }
  }
  
  Serial.println("Timeout - No headers");
  return false;
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