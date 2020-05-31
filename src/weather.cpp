#include <math.h>
#include <time.h>

#include "weather.h"

namespace WeatherAPI
{
  void parseCurrentWeather(DynamicJsonDocument &doc, Weather *weather)
  {
    unsigned long long current_weather_time = doc["currentWeather"]["time"].as<unsigned long long>();
    weather->current.time = static_cast<uint32_t>(current_weather_time / 1000);
    weather->current.icon = doc["currentWeather"]["icon"].as<unsigned char>();
    weather->current.temperature = doc["currentWeather"]["temperature"].as<float>();
  }

  void parseForecasts(DynamicJsonDocument &doc, Weather *weather, time_t current_time)
  {  
    struct tm current = *localtime(&current_time);

    char today_str[12];
    strftime(today_str, sizeof(today_str), "%Y-%m-%d", &current);

    JsonArray forecasts = doc["forecast"];
    
    bool started = false;
    int i_forecast = 0;
    for(JsonObject f : forecasts) {
      const char *date = f["dayDate"].as<char *>();

      if (!started && strcmp(date, today_str) != 0) continue;
      started = true;

      struct tm tm = {0};
      strptime(date, "%Y-%m-%d", &tm);
      uint8_t weekDay = static_cast<uint8_t>(tm.tm_wday) + 1; // 1 = sunday

      uint8_t icon = static_cast<uint8_t>(f["iconDay"].as<unsigned char>());
      float temperatureMax = f["temperatureMax"].as<float>();
      float temperatureMin = f["temperatureMin"].as<float>();
      float precipitation = f["precipitation"].as<float>();

      WeatherForecast forecast = {.weekDay = weekDay, .icon = icon, .tempMax = temperatureMax, .tempMin = temperatureMin, .precipitation = precipitation};
      weather->forecasts[i_forecast] = forecast;
      i_forecast++;

      if (i_forecast >= NUM_FORECASTS) break; // stop when forecasts array is full
    }
  }

  void parseGraph(DynamicJsonDocument &doc, Weather *weather, time_t current_time)
  {
    JsonArray icons = doc["graph"]["weatherIcon3h"];
    JsonArray temperatureMean1h = doc["graph"]["temperatureMean1h"];
    JsonArray precipitation1h = doc["graph"]["precipitation1h"];
    JsonArray precipitation10min = doc["graph"]["precipitation10m"];

    // round to 
    struct tm rounded = *localtime(&current_time);
    rounded.tm_sec = 0;
    rounded.tm_min = 0;
    rounded.tm_hour -= rounded.tm_hour % 3; // round down to three hours
    time_t rounded_time = mktime(&rounded);

    // Graph
    unsigned long long start_s = doc["graph"]["start"].as<unsigned long long>();
    time_t start = static_cast<time_t>(start_s / 1000);

    // Start for low resolution (precipitation 1h)
    unsigned long long start_l = doc["graph"]["startLowResolution"].as<unsigned long long>();
    time_t start_low = static_cast<time_t>(start_l / 1000);
    
    bool started = false;
    for (int idx = 0, i_3h = 0; idx < NUM_3H && i_3h < icons.size(); i_3h++)
    {
      if (!started && start + i_3h * 10800 < rounded_time) continue;
      started = true;

      weather->icons[idx] = icons[i_3h].as<unsigned char>();
      idx++;
    }

    started = false;
    for (int idx = 0, i_1h = 0; idx < NUM_1H && i_1h < temperatureMean1h.size(); i_1h++)
    {
      if (!started && start + i_1h * 3600 < rounded_time) continue;
      started = true;

      weather->temperatures[idx] = temperatureMean1h[i_1h].as<float>();
      idx++;
    }

    started = false;
    for (int idx = 0, i_1h = 0; idx < NUM_1H && i_1h < precipitation1h.size(); i_1h++)
    {
      if (!started && start_low + i_1h * 3600 < rounded_time) continue;
      started = true;

      weather->precipitation1h[idx] = precipitation1h[i_1h].as<float>();
      idx++;
    }

    started = false;
    for (int idx = 0, i_10min = 0; idx < NUM_10MIN && i_10min < precipitation10min.size(); i_10min++)
    {
      if (!started && start + i_10min * 600 < rounded_time) continue;
      started = true;
    
      weather->precipitation10min[idx] = precipitation10min[i_10min].as<float>();
      idx++;
    }

    weather->start = rounded_time;
    weather->start_low = rounded_time > start_low ? rounded_time : start_low;
  }

  bool parseWeather(DynamicJsonDocument &doc, Weather *weather, time_t current_time)
  {
    // assume weather was reset to {0}

    parseCurrentWeather(doc, weather);
    parseForecasts(doc, weather, current_time);
    parseGraph(doc, weather, current_time);

    return true;
  }
}