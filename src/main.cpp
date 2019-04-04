#include <time.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "weather.h"
#include "display.h"
#include "webrequest.h"
#include "wifi_login.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

#define MAX_TIME_SYNC 60

RTC_DATA_ATTR Weather weather_save;

bool requestWeather(Weather *weather) {
  // WebRequest *request = new WebRequest();
  // bool success = request->requestWeather();
  // if (!success) {
  //   return;
  // }

  DynamicJsonDocument doc(20000);

  // Deserialize the JSON document    
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }

  JsonArray forecast = doc["forecast"];
  
  unsigned long long start_s = doc["graph"]["start"].as<unsigned long long>();

  Serial.println(static_cast<unsigned long>(start_s / 1000));

  time_t start = static_cast<time_t>(start_s / 1000);

  tm tm_start = *localtime(&start);

  char buffer[80];
  strftime(buffer, 80, "Start %H:%M:%S on %Y-%m-%d - A %A", &tm_start);
  Serial.println(buffer);

  JsonArray icons = doc["graph"]["weatherIcon3h"];
  JsonArray temperatureMean1h = doc["graph"]["temperatureMean1h"];
  JsonArray precipitationMean1h = doc["graph"]["precipitationMean1h"];
  
  int i_forecast = 0;
  for(JsonObject f : forecast) {
    const char *date = f["dayDate"].as<char *>();
    uint8_t icon = static_cast<uint8_t>(f["iconDay"].as<unsigned char>());
    float temperatureMax = f["temperatureMax"].as<float>();
    float temperatureMin = f["temperatureMin"].as<float>();
    float precipitation = f["precipitation"].as<float>();

    struct tm tm;
    strptime(date, "%Y-%m-%d", &tm);

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

    // Make sure to start on the correct day
    // otherwise continue

    uint8_t weekDay = static_cast<uint8_t>(tm.tm_wday) + 1; // 1 = sunday

    WeatherForecast forecast = {.weekDay = weekDay, .icon = icon, .tempMax = temperatureMax, .tempMin = temperatureMin, .precipitation = precipitation};
    weather->forecasts[i_forecast] = forecast;
    i_forecast++;

    if (i_forecast >= NUM_FORECASTS) break; // stop when forecasts array is full
  }

  int hour = tm_start.tm_hour;
  
  bool started = false;
  int i_hour = 0;
  for (int i = 0; i < 48; i++) {

    if (!started && (hour + i) % 24 != HOUR_START) continue;
    started = true;
  
    int i_3h = floorf((float)i_hour / 3);
    
    // Initialize all to zero 
    weather->temperatures[i_hour] = 0.f;
    weather->precipitation[i_hour] = 0.f;
    if (i_hour % 3 == 0 && i_3h < NUM_3H) weather->icons[i_3h] = 0;

    if (icons.size() > (float)i / 3 && i_hour % 3 == 0 && i_3h < NUM_3H) {
      weather->icons[i_3h] = static_cast<uint8_t>(icons[(int)floorf((float)i / 3)].as<unsigned char>());
    }

    if (temperatureMean1h.size() > i && i_hour < NUM_1H) {
      weather->temperatures[i_hour] = temperatureMean1h[i].as<float>();
    }

    if (temperatureMean1h.size() > i && i_hour < NUM_1H) {
      weather->precipitation[i_hour] = precipitationMean1h[i].as<float>();
    }

    i_hour++;
    
    if (i_hour >= NUM_1H && i_3h >= NUM_3H) break; // Stop when both 1h & 3h arrays are full
  }

  return true;
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
  delay(100);
  Serial.println("");

  WebRequest web = WebRequest();

  Display *display = new Display();

  bool success = false;
  Weather weather = weather_save;

  // check if we should update
  time_t current_time = time(NULL); // wrong date will be much higher than last_update = 0 as well
  if (current_time - weather.last_update >= 24 * 3600) {
    
    tryUpdateTime(&web);
    success = requestWeather(&weather);
    if (success) {
      weather.last_update = time(NULL);
      weather_save = weather;
    }
  }

  display->renderWeather(weather);
  display->draw();
  delete display;

  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // esp_deep_sleep_start();
}

void loop() {

}