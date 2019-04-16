#ifndef WeatherForecast_h
#define WeatherForecast_h

#include <stdint.h>
#include <Arduino.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#define JSON_CAPACITY 32768

#define HOUR_START 6
#define NUM_FORECASTS 6
#define NUM_1H 19
#define NUM_3H 7

struct WeatherForecast {
    uint8_t weekDay;
    uint8_t icon;
    float tempMax;
    float tempMin;
    float precipitation;
};

struct CurrentWeather {
    uint32_t time;
    uint8_t icon;
    float temperature;
};

struct Weather {
    uint32_t last_update;
    uint8_t icons[NUM_3H];
    float precipitation[NUM_1H];
    float temperatures[NUM_1H];
    CurrentWeather current;
    WeatherForecast forecasts[NUM_FORECASTS];
};

namespace WeatherAPI {
    bool parseWeather(DynamicJsonDocument &doc, Weather *weather);
}

#endif /* WeatherForecast_h */
