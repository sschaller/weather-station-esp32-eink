#include "weather.h"
#include "display.h"
#include "webrequest.h"
#include "wifi_login.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

#define MAX_TIME_SYNC 60
#define UPDATE_FREQUENCY 3 * 3600

RTC_DATA_ATTR Weather weather_save = {0};
RTC_DATA_ATTR char tz[33] = {0};

bool requestWeather(WebRequest &web, Weather *weather, time_t current_time) {
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
  
  // Set local time so old data will be used
  time_t prev_time = current_time;
  current_time = static_cast<time_t>(doc["currentWeather"]["time"].as<unsigned long long>() / 1000);
#endif

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }

  // serializeJson(doc, Serial);

  bool success2 = WeatherAPI::parseWeather(doc, weather, current_time);
  
  #if !USE_WEATHER_API
  // Set start times to rounded current time to make up for old data set
  struct tm rounded = *localtime(&prev_time);
  rounded.tm_sec = 0;
  rounded.tm_min = 0;
  rounded.tm_hour -= rounded.tm_hour % 3; // round down to three hours
  time_t rounded_time = mktime(&rounded);

  weather->start = rounded_time;
  weather->start_low = rounded_time;
  weather->current.time = rounded_time;
  #endif

  return success2;
}

bool tryUpdateTime(WebRequest *web, time_t &current_time) {
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
  
  current_time = time(NULL);
  
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
    } else if(!tryUpdateTime(&web, current_time)) {
      error = UpdateError::ETime;
    } else if(!requestWeather(web, &weather, current_time)) {
      error = UpdateError::EWeather;
    } else {
      weather_save = weather;
    }
  }

  current = *localtime(&current_time);

  display->initialize(true);

  rounded_hour = current.tm_hour + (int)roundf((float)current.tm_min / 60);
  current.tm_hour = rounded_hour;
  current.tm_min = 0;
  current.tm_sec = 0;
  time_t rounded_time = mktime(&current);
  int offset_hour = (rounded_time - weather.start) / 3600;

  display->renderWeather(weather, rounded_hour, offset_hour);

  if (error != UpdateError::ENone) {
    display->renderError(error);
  }

  display->draw();
  delete display;

  // wake up every hour (at :05)
  uint64_t time_to_sleep = 300 + 3600 - time(NULL) % 3600;

  esp_sleep_enable_timer_wakeup(time_to_sleep * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {

}