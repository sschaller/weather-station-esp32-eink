#ifndef WeatherForecast_h
#define WeatherForecast_h

#define HOUR_START 6
#define NUM_FORECASTS 5
#define NUM_1H 19
#define NUM_3H 7

struct WeatherForecast {
    uint8_t weekDay;
    uint8_t icon;
    float tempMax;
    float tempMin;
    float precipitation;
};

struct Weather {
    uint32_t last_update;
    uint8_t icons[NUM_3H];
    float precipitation[NUM_1H];
    float temperatures[NUM_1H];
    WeatherForecast forecasts[NUM_FORECASTS];
};

#endif /* WeatherForecast_h */
