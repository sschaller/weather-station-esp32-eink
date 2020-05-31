#ifndef WeatherForecast_h
#define WeatherForecast_h

#include <stdint.h>
#include <Arduino.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#define JSON_CAPACITY 32768

#define HOUR_START 6
#define NUM_FORECASTS 6
#define NUM_3H 11
#define NUM_1H 33
#define NUM_10MIN 42

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
    uint32_t start;
    uint32_t start_low;
    uint8_t icons[NUM_3H];
    float precipitation1h[NUM_1H];
    float precipitation10min[NUM_10MIN];
    float temperatures[NUM_1H];
    CurrentWeather current;
    WeatherForecast forecasts[NUM_FORECASTS];
};

namespace WeatherAPI {
    bool parseWeather(DynamicJsonDocument &doc, Weather *weather, time_t current_time);
}

#endif /* WeatherForecast_h */
