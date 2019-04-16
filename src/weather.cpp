#include <math.h>
#include <time.h>

#include "weather.h"

namespace WeatherAPI {
  bool parseWeather(DynamicJsonDocument &doc, Weather *weather) {

    // Current Weather
    unsigned long long current_weather_time = doc["currentWeather"]["time"].as<unsigned long long>();
    weather->current.time = static_cast<uint32_t>(current_weather_time / 1000);
    weather->current.icon = doc["currentWeather"]["icon"].as<unsigned char>();
    weather->current.temperature = doc["currentWeather"]["temperature"].as<float>();

    // Graph
    unsigned long long start_s = doc["graph"]["start"].as<unsigned long long>();

    time_t current_time = time(NULL);
    struct tm current = *localtime(&current_time);

    struct tm day = current;
    day.tm_sec = 0;
    day.tm_min = 0;
    day.tm_hour = 0;

    time_t day_time = mktime(&day);

    time_t start = static_cast<time_t>(start_s / 1000);
    struct tm tm_start = *localtime(&start);

    char date_buffer[12];
    strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", &tm_start);

    JsonArray forecasts = doc["forecast"];
    JsonArray icons = doc["graph"]["weatherIcon3h"];
    JsonArray temperatureMean1h = doc["graph"]["temperatureMean1h"];
    JsonArray precipitationMean1h = doc["graph"]["precipitationMean1h"];
    
    int i_forecast = 0;
    bool started = false;
    for(JsonObject f : forecasts) {
      const char *date = f["dayDate"].as<char *>();

      if (!started && strcmp(date, date_buffer) != 0) continue;
      started = true;

      struct tm tm = {0};
      strptime(date, "%Y-%m-%d", &tm);

      uint8_t icon = static_cast<uint8_t>(f["iconDay"].as<unsigned char>());
      float temperatureMax = f["temperatureMax"].as<float>();
      float temperatureMin = f["temperatureMin"].as<float>();
      float precipitation = f["precipitation"].as<float>();

      uint8_t weekDay = static_cast<uint8_t>(tm.tm_wday) + 1; // 1 = sunday

      WeatherForecast forecast = {.weekDay = weekDay, .icon = icon, .tempMax = temperatureMax, .tempMin = temperatureMin, .precipitation = precipitation};
      weather->forecasts[i_forecast] = forecast;
      i_forecast++;

      if (i_forecast >= NUM_FORECASTS) break; // stop when forecasts array is full
    }

    int hour = tm_start.tm_hour;
    
    started = false;
    int i_hour = 0;
    for (int i = 0; i < 48; i++) {

      if (!started && (start + i * 3600 < day_time || (hour + i) % 24 != HOUR_START)) continue;
      started = true;
    
      int i_3h = floorf((float)i_hour / 3);

      if (i_hour % 3 == 0 && i_3h < NUM_3H) {
        weather->icons[i_3h] = 0;
        if (icons.size() > (int)floorf((float)i / 3)) {
          weather->icons[i_3h] = icons[(int)floorf((float)i / 3)].as<unsigned char>();
        }
      }

      if (i_hour < NUM_1H) {
        weather->temperatures[i_hour] = 0.f;
        if (temperatureMean1h.size() > i) {
          weather->temperatures[i_hour] = temperatureMean1h[i].as<float>();
        }
      }

      if (i_hour < NUM_1H) {
        weather->precipitation[i_hour] = 0.f;
        if (temperatureMean1h.size() > i) {
          weather->precipitation[i_hour] = precipitationMean1h[i].as<float>();
        }
      }

      i_hour++;
      i_3h = floorf((float)i_hour / 3);
      if (i_hour >= NUM_1H && i_3h >= NUM_3H) break; // Stop when both 1h & 3h arrays are full
    }

    weather->last_update = current_time;

    return true;
  }
}