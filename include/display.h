#ifndef Display_h
#define Display_h

#include <SPI.h>
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


        void renderTime();
        void draw();

};

#endif /* Display_h */
