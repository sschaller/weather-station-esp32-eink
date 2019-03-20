#ifndef Display_h
#define Display_h

#include "weather.h"

class Display {
    bool initialized;
    Epd epd;

    Display();

    bool initialize();
    void renderWeatherForecast(WeatherForecast[] forecast);
}

#endif /* Display_h */
