#include "weather.h"
#include "display.h"
#include "webrequest.h"
#include "wifi_login.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

#define MAX_TIME_SYNC 60
#define UPDATE_FREQUENCY 3 * 3600

RTC_DATA_ATTR Weather weather_save = {0};
RTC_DATA_ATTR char tz[33] = {0};

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
    Serial.println("Failed to get time");
    return false;
  }

  // copy TZ environment variable to RTC RAM
  strcpy(tz, getenv("TZ"));
  
  return true;
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  
  WebRequest web = WebRequest();
  Display *display = new Display();
  Weather weather = weather_save;

  if (strlen(tz) > 0) {
    // if set load TZ environment variable from RTC RAM
    setenv("TZ", tz, 1);
    tzset();
  }

  // check if we should update
  time_t current_time = time(NULL); // wrong date will be much higher than last_update = 0 as well
  struct tm current = *localtime(&current_time);

  int rounded_hour = current.tm_hour + (int)roundf((float)current.tm_min / 60);

  UpdateError error = UpdateError::ENone;
  if (current_time == 0 || rounded_hour % 3 == 0) {
    if (!web.connect()) {
      error = UpdateError::EConnection;
    }
    if (error == UpdateError::ENone && !tryUpdateTime(&web)) {
      error = UpdateError::ETime;
    }
    if (error == UpdateError::ENone && !requestWeather(web, &weather)) {
      error = UpdateError::EWeather;
    }
    if (error == UpdateError::ENone) {
      weather_save = weather;
    } else {
      // display UpdateError
      Serial.print("Update Error: ");
      Serial.println(error);
    }
  }

  current_time = time(NULL);
  current = *localtime(&current_time);

  display->initialize(true);

  // display->renderText(String(rounded_hour).c_str(), 180, 80);
  // display->renderTime(original_time, 220, 80);

  rounded_hour = current.tm_hour + (int)roundf((float)current.tm_min / 60);
  display->renderWeather(weather, rounded_hour);

  if (error != UpdateError::ENone) {
    display->renderError(error);
  }

  // display->renderTime(current_time, 280, 80);
  // display->renderTime(weather.last_update, 340, 80);

  // display->print();
  display->draw();
  delete display;

  // wake up every hour (at :05)
  uint64_t time_to_sleep = 300 + 3600 - time(NULL) % 3600;

  esp_sleep_enable_timer_wakeup(time_to_sleep * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {

}