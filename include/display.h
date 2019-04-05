#ifndef Display_h
#define Display_h

#include "epd4in2.h"
#include "epdpaint.h"

#include "weather.h"

class Display {
    bool initialized;
    Epd epd;

    const int width = 400; //width should be the multiple of 8
    const int height = 300;

    unsigned char *buffer;
    Paint *paint;

    public:
        ~Display();
        bool initialize(bool clear_buffer);
        void calculateResolution(float &y_lower, float &y_upper, float &step);
        void renderIcon(uint8_t icon_type, int x, int y);

        void renderWeatherForecast(const WeatherForecast *forecast, int num_forecasts);
        void renderTemperatureCurve(float *temperatures, int num_points);
        void renderPrecipitation(float *precipitation, int num_points);
        void renderDailyGraph(float *temperatures, float *precipitation, int num_points, int current_hour);
        void render24hIcons(uint8_t *icons, int num_steps);
        void renderWeather(Weather weather, int current_hour);
        void renderTime(time_t t, int x, int y);

        void draw();

};

#endif /* Display_h */
