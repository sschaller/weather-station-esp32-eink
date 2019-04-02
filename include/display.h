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
        void renderWeatherForecast(WeatherForecast *forecast, int num_forecasts);
        void renderIcon(int icon_type, int x, int y);

        void renderTemperatureCurve(float *hours, float *temperatureMean, float y_min, float y_max);
        void renderTemperatureCurves(float *temperatureMean);
        void render24hIcons(int *icons);
        void renderTime();
        void draw();

};

#endif /* Display_h */
