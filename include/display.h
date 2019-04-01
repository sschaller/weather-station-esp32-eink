#ifndef Display_h
#define Display_h

#include "epd4in2.h"
#include "epdpaint.h"

#include "weather.h"

class Display {
    bool initialized;
    Epd epd;

    public:
        ~Display();
        bool initialize(bool clear_buffer);
        void renderWeatherForecast(WeatherForecast *forecast, int num_forecasts);
        void renderIcon(Paint paint, int icon_type, int x, int y);

        void renderTemperatureCurve(Paint paint, float *hours, float *temperatureMean, float y_min, float y_max);
        void renderTemperatureCurves(float *temperatureMean, float *temperatureMin, float *temperatureMax);
        void render24hIcons(int *icons);
        void renderTime();
        void draw();

};

#endif /* Display_h */
