#include <time.h>

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
  int start = doc["graph"]["start"];
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

  display->renderWeatherForecast(forecasts, forecast.size());
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
  
  // display->renderTime();
  display->draw();
  delete display;

  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // esp_deep_sleep_start();
}

void loop() {

}