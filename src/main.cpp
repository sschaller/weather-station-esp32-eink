#include <time.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "weather.h"
#include "display.h"
#include "webrequest.h"
#include "wifi_login.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR bool first_time = true;

void requestWeather(Display *display) {
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
    return;
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
  JsonArray temperatureMin1h = doc["graph"]["temperatureMin1h"];
  JsonArray temperatureMax1h = doc["graph"]["temperatureMax1h"];
  JsonArray temperatureMean1h = doc["graph"]["temperatureMean1h"];
  JsonArray precipitationMean1h = doc["graph"]["precipitationMean1h"];

  WeatherForecast *forecasts = (WeatherForecast *) malloc(forecast.size() * sizeof(WeatherForecast));
  int forecasts_size = 0;
  
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


    struct tm tm;
    strptime(date, "%Y-%m-%d", &tm);

    int weekDay = tm.tm_wday + 1; // 1 = sunday

    WeatherForecast forecast = {.weekDay = weekDay, .icon = icon, .tempMax = temperatureMax, .tempMin = temperatureMin, .precipitation = precipitation};
    forecasts[forecasts_size] = forecast;
    forecasts_size++;
  }

  int hour = tm_start.tm_hour;
  bool started = false;
  int hours = 0;

  int icons3h[24];
  float temperatureMean[24];
  float temperatureMax[24];
  float temperatureMin[24];
  for (int i = 0; i < 48; i++) {

    if (!started && (hour + i) % 24 < 6) continue;
    started = true;
  
    if (icons.size() > i && hours % 3 == 0) {
      int icon_idx = floorf((float)hours / 3);
      icons3h[icon_idx] = icons[i].as<int>();
    }
    if (temperatureMean1h.size() > i) {
      temperatureMean[i] = temperatureMean1h[i].as<float>();
    } else {
      temperatureMean[i] = 0.f;
    }

    hours++;
    if ((hour + i) % 24 == 1) break; // stop at 1 AM
  }

  Serial.println(hours);

  display->renderWeatherForecast(forecasts, forecast.size());

  display->renderTemperatureCurves(temperatureMean);

  display->render24hIcons(icons3h);

  delete[] forecasts;
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  delay(100);
  Serial.println("");

  Display *display = new Display();
  
  if (first_time) {
    requestWeather(display);
    first_time = false;
  }
  
  display->draw();
  delete display;

  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // esp_deep_sleep_start();
}

void loop() {

}