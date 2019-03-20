#include <SPI.h>
#include "epd4in2.h"
#include "epdpaint.h"

#include "display.h"

bool Display::initialize() {
    if(initialized) {
        return true;
    }

    Serial.print("Initialize e-Paper... ");

    if (epd.Init() != 0) {
        Serial.println("failed");
        return;
    }
    
    Serial.println("success");

    /* This clears the SRAM of the e-paper display */
    epd.ClearFrame();
}

void Display::renderWeatherForecast(WeatherForecast[] forecast)
{
    if (!initialize()) {
        return;
    }


}

void Display::draw()
{
    if (!initialized) {
        // Not initialized
        Serial.println("Nothing to draw");
        return;
    }
    /* This displays the data from the SRAM in e-Paper module */
    epd.DisplayFrame();

    /* Deep sleep */
    epd.Sleep();

    Serial.println("Finished e-Paper");
}