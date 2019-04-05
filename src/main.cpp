#include "weather.h"
#include "display.h"
#include "webrequest.h"
#include "wifi_login.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

#define MAX_TIME_SYNC 60
#define UPDATE_FREQUENCY 3 * 3600

RTC_DATA_ATTR Weather weather_save = {0};

bool requestWeather(WebRequest &web, Weather *weather) {
  DynamicJsonDocument doc(JSON_CAPACITY);
  
#if USE_WEATHER_API
  bool success = web.requestWeather();
  if (!success) {
    return false;
  }
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, web.client);
#else
  DeserializationError error = deserializeJson(doc, json);
#endif

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }

  // serializeJson(doc, Serial);

  return WeatherAPI::parseWeather(doc, weather);
}

bool tryUpdateTime(WebRequest *web) {
  web->connect();
  
  configTime(3600, 3600, "pool.ntp.org");

  struct tm info;

  // Give it up to MAX_TIME_SYNC seconds to synchronize
  if(!getLocalTime(&info, MAX_TIME_SYNC * 1000)){
    return false;
  }

  return true;
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  
  WebRequest web = WebRequest();
  Display *display = new Display();
  Weather weather = weather_save;

  // check if we should update
  time_t current_time = time(NULL); // wrong date will be much higher than last_update = 0 as well

  Serial.print(String(current_time) + " " + String(weather.last_update));
  if (current_time == 0 || current_time - weather.last_update >= UPDATE_FREQUENCY) {
    
    tryUpdateTime(&web);
    bool success = requestWeather(web, &weather);
    if (success) {
      weather_save = weather;
    }
  }

  current_time = time(NULL);
  struct tm current = *localtime(&current_time);

  display->initialize(true);
  display->renderWeather(weather, current.tm_hour);

  display->renderTime(current_time, 280, 80);
  display->renderTime(weather.last_update, 340, 80);

  display->draw();
  delete display;

  // wake up every hour (at :05)
  uint64_t time_to_sleep = 300 + 3600 - time(NULL) % 3600;

  esp_sleep_enable_timer_wakeup(time_to_sleep * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {

}