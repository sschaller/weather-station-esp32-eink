#include "display.h"
#include "data.h"
#include "spline.h"

#define LABEL_OFFSET 40
#define COLORED     0
#define UNCOLORED   1

static char buffer[3+1];
char* dayShortStr(uint8_t day) {
   uint8_t index = day*3;
   for (int i=0; i < 3; i++)      
      buffer[i] = pgm_read_byte(&(dayShortNames_P[index + i]));  
   buffer[3] = 0; 
   return buffer;
}

bool Display::initialize(bool clear_buffer) {
    if(initialized) {
        return true;
    }

    Serial.print("Initialize e-Paper... ");

    if (epd.Init() != 0) {
        Serial.println("failed");
        return false;
    }
    
    Serial.println("success");

    /* This clears the SRAM of the e-paper display */
    if (clear_buffer) epd.ClearFrame();

    initialized = true;

    return true;
}

void Display::renderWeatherForecast(WeatherForecast *forecasts, int num_forecasts)
{
    if (!initialize(true)) {
        return;
    }

    unsigned char image[5200];
    Paint paint(image, 400, 104);    //width should be the multiple of 8

    paint.Clear(UNCOLORED);

    int i;
    for (i = 0; i < num_forecasts && i < 5; i++) {
        int x = 80 * i;
        WeatherForecast f = forecasts[i];
        renderIcon(paint, f.icon, x + 15, 0);

        String temp = String(String(f.tempMin, DEC) + "|" + String(f.tempMax, DEC));
        int len = temp.length() * 11;
        if(f.tempMin < 0) len -= 5;
        int start = (80 - len) / 2;

        paint.DrawStringAt(x + start, 52, temp.c_str(), &Font16, COLORED);

        
        paint.DrawStringAt(x + 15, 76, dayShortStr(f.weekDay), &Font24, COLORED);
    }

    epd.SetPartialWindow(paint.GetImage(), 0, 200, paint.GetWidth(), paint.GetHeight());
}

void Display::renderIcon(Paint paint, int icon_type, int x, int y)
{
    // Drawing area will be overwritten in places of icons (no need to clear)

    bool is_sunny = (icon_type == 1 || icon_type == 26 || icon_type == 27);
    bool is_mostly_sunny = (icon_type == 2);
    bool is_mostly_cloudy = (icon_type == 3 || icon_type == 4 || (icon_type >= 6 && icon_type <= 13) || (icon_type >= 29 && icon_type <= 34));
    bool is_cloudy = (icon_type == 5 || (icon_type >= 14 && icon_type <= 25) || icon_type == 35);
    bool is_dark_cloud = ((icon_type >= 4 && icon_type <= 11) || (icon_type >= 13 && icon_type <= 25) || icon_type == 33 || icon_type == 34);    

    // calculate offset for different cloud versions
    int offset_cloud_x = is_cloudy ? 5 : 10;

    if (is_dark_cloud) {
        // draw background pattern for cloud first    
        paint.DrawBuffer(CLOUD_DARK, CLOUD_DARK_SIZE, x + offset_cloud_x, y + 11, COLORED);
    }

    // Draw template according to type
    if (is_sunny) {
        paint.DrawBuffer(SUNNY, SUNNY_SIZE, x + 8, y + 8, COLORED);
    } else if (is_mostly_sunny) {
        paint.DrawBuffer(MOSTLY_SUNNY, MOSTLY_SUNNY_SIZE, x + 4, y + 2, COLORED);
    } else if (is_mostly_cloudy) {
        paint.DrawBuffer(MOSTLY_CLOUDY, MOSTLY_CLOUDY_SIZE, x, y, COLORED);
    } else if (is_cloudy) {
        paint.DrawBuffer(CLOUDY, CLOUDY_SIZE, x + 5, y + 11, COLORED);
    }

    // custom types
    if (icon_type == 26) {
        // draw high fog, no transparency
        paint.DrawBufferOpaque(FOG_TOP, FOG_TOP_SIZE, x + 5, y + 7, COLORED);
    } else if (icon_type == 27) {
        // draw low fog, no transparency
        paint.DrawBufferOpaque(FOG_BOTTOM, FOG_BOTTOM_SIZE, x + 5, y + 26, COLORED);
    } else if (icon_type == 28) {
        // fog - draw both
        paint.DrawBuffer(FOG_TOP, FOG_TOP_SIZE, x + 5, y + 12, COLORED);
        paint.DrawBuffer(FOG_BOTTOM, FOG_BOTTOM_SIZE, x + 5, y + 26, COLORED);
    } else if (icon_type == 12) {
        // 1 bolt
        paint.DrawBufferAlpha(BOLT_1, BOLT_1_ALPHA, BOLT_1_SIZE, x + offset_cloud_x + 15, y + 28, COLORED);
    } else if (icon_type == 13 || icon_type == 23) {
        // 1 bolt, 2 rain
        paint.DrawBufferAlpha(BOLT_1_RAIN_2, BOLT_1_RAIN_2_ALPHA, BOLT_1_RAIN_2_SIZE, x + offset_cloud_x + 9, y + 28, COLORED);
    } else if (icon_type == 24) {
        // 2 bolts, 2 rain
        paint.DrawBufferAlpha(BOLT_2_RAIN_2, BOLT_2_RAIN_2_ALPHA, BOLT_2_RAIN_2_SIZE, x + offset_cloud_x + 9, y + 28, COLORED);
    } else if (icon_type == 25) {
        // 2 bolts, 4 rain
        paint.DrawBufferAlpha(BOLT_2_RAIN_4, BOLT_2_RAIN_4_ALPHA, BOLT_2_RAIN_4_SIZE, x + offset_cloud_x + 3, y + 28, COLORED);
    } else if (icon_type == 6 || icon_type == 29) {
        // 1 rain
        paint.DrawBufferAlpha(RAIN_1, RAIN_1_ALPHA, RAIN_1_SIZE, x + offset_cloud_x + 15, y + 32, COLORED);
    } else if (icon_type == 9 || icon_type == 14 || icon_type == 32) {
        // 2 rain
        paint.DrawBufferAlpha(RAIN_2, RAIN_2_ALPHA, RAIN_2_SIZE, x + offset_cloud_x + 12, y + 32, COLORED);
    } else if (icon_type == 17 || icon_type == 33) {
        // 3 rain
        paint.DrawBufferAlpha(RAIN_3, RAIN_3_ALPHA, RAIN_3_SIZE, x + offset_cloud_x + 11, y + 31, COLORED);
    } else if (icon_type == 20) {
        // 4 rain
        paint.DrawBufferAlpha(RAIN_4, RAIN_4_ALPHA, RAIN_4_SIZE, x + offset_cloud_x + 7, y + 30, COLORED);
    } else if (icon_type == 8 || icon_type == 30) {
        // 1 snow
        paint.DrawBufferAlpha(SNOW_1, SNOW_1_ALPHA, SNOW_1_SIZE, x + offset_cloud_x + 14, y + 33, COLORED);
    } else if (icon_type == 11 || icon_type == 16) {
        // 2 snow
        paint.DrawBufferAlpha(SNOW_2, SNOW_2_ALPHA, SNOW_2_SIZE, x + offset_cloud_x + 11, y + 32, COLORED);
    } else if (icon_type == 19 || icon_type == 34) {
        // 3 snow
        paint.DrawBufferAlpha(SNOW_3, SNOW_3_ALPHA, SNOW_3_SIZE, x + offset_cloud_x + 11, y + 31, COLORED);
    } else if (icon_type == 22) {
        // 4 snow
        paint.DrawBufferAlpha(SNOW_4, SNOW_4_ALPHA, SNOW_4_SIZE, x + offset_cloud_x + 4, y + 33, COLORED);
    } else if (icon_type == 7 || icon_type == 15 || icon_type == 31) {
        // 1 rain 1 snow
        paint.DrawBufferAlpha(RAIN_1_SNOW_1, RAIN_1_SNOW_1_ALPHA, RAIN_1_SNOW_1_SIZE, x + offset_cloud_x + 12, y + 31, COLORED);
    } else if (icon_type == 10) {
        // 2 rain 1 snow
        paint.DrawBufferAlpha(RAIN_2_SNOW_1, RAIN_2_SNOW_1_ALPHA, RAIN_2_SNOW_1_SIZE, x + offset_cloud_x + 6, y + 31, COLORED);
    } else if (icon_type == 18) {
        // 2 rain 2 snow
        paint.DrawBufferAlpha(RAIN_2_SNOW_2, RAIN_2_SNOW_2_ALPHA, RAIN_2_SNOW_2_SIZE, x + offset_cloud_x + 7, y + 30, COLORED);
    } else if (icon_type == 21) {
        // 3 rain 3 snow
        paint.DrawBufferAlpha(RAIN_3_SNOW_3, RAIN_3_SNOW_3_ALPHA, RAIN_3_SNOW_3_SIZE, x + offset_cloud_x + 6, y + 31, COLORED);
    }
}

void Display::renderTemperatureCurve(Paint paint, float *hours, float *temperatureMean, float y_min, float y_max) {
    float range = fabs(y_max - y_min);

    float y2[24];
    spline(hours, temperatureMean, 24, y2);

    int limit_x = paint.GetWidth() - LABEL_OFFSET;

    float y;
    int y_n;
    for (int i = 0; i < limit_x; i++) {
        splint(hours, temperatureMean, y2, 24, 24.f * i / limit_x, &y);

        y_n = floorf(56.f * (1.f - (y - y_min) / range));
        paint.DrawPixel(LABEL_OFFSET + i, 7 + y_n, COLORED);
        paint.DrawPixel(LABEL_OFFSET + i, 8 + y_n, COLORED);
    }
}

void Display::renderTemperatureCurves(float *temperatureMean, float *temperatureMin, float *temperatureMax) {
    unsigned char buffer[3600];
    Paint paint(buffer, 400, 72); //width should be the multiple of 8
    paint.Clear(UNCOLORED);

    float hours[24];
    float y_min = 100.f;
    float y_max = -100.f;

    for (int i = 0; i < 24; i++) {
        hours[i] = static_cast<float>(i);
        
        if(temperatureMean[i] < y_min) y_min = temperatureMean[i];
        if(temperatureMean[i] > y_max) y_max = temperatureMean[i];

        // if(temperatureMin[i] < y_min) y_min = temperatureMin[i];
        // if(temperatureMax[i] > y_max) y_max = temperatureMax[i];
    }

    float step = 5.f;

    float y_upper = 5 * ceilf(y_max / 5);
    float y_lower = 5 * floorf(y_min / 5);

    float range = y_upper - y_lower;

    if(range > 20) {
        step = 10.f;

        y_upper = 10 * ceilf(y_max / 10);
        y_lower = 10 * floorf(y_min / 10);

        range = y_upper - y_lower;
    }

    Serial.print(y_min);
    Serial.print(" ");
    Serial.println(y_max);

    for (int i = 0; i * step <= range; i++) {

        int y = static_cast<int>(floorf(56.f * (1.f - (i * step) / range)));
        int y_text = static_cast<int>(i * step + y_lower);
        String t = String(y_text);
        int text_offset = t.length() * 11;
        if (y_text < 0) text_offset -= 6;

        paint.DrawHorizontalLine(LABEL_OFFSET, y + 6, paint.GetWidth() - LABEL_OFFSET, COLORED);
        paint.DrawStringAt(LABEL_OFFSET - 4 - text_offset, y, t.c_str(), &Font16, COLORED);
    }

    renderTemperatureCurve(paint, hours, temperatureMean, y_lower, y_upper);
    // renderTemperatureCurve(paint, hours, temperatureMin, y_lower, y_upper);
    // renderTemperatureCurve(paint, hours, temperatureMax, y_lower, y_upper);

    epd.SetPartialWindow(paint.GetImage(), 0, 120, paint.GetWidth(), paint.GetHeight());
}

void Display::render24hIcons(int *icons)
{
    unsigned char icon_buffer[2400];
    Paint paint(icon_buffer, 400, 48);

    int start = 6;
    int end = 24;
    int step = 3;

    int steps = (end - start) / step;
    int part = (paint.GetWidth() - LABEL_OFFSET) / steps;

    for (int i = 0; i <= steps; i++) {

        int hour = start + i * step;

        int part = (paint.GetWidth() - LABEL_OFFSET) / steps;

        String text = String(hour) + String(":00");
        int offset = (text.length() * 14) / 2;
        paint.DrawStringAt(LABEL_OFFSET + i * part + part / 2 - offset, 0, text.c_str(), &Font20, COLORED);
    }
}

void Display::renderTime()
{
    if (!initialize(true)) {
        return;
    }

    unsigned char icon_buffer[2500];
    Paint paint(icon_buffer, 400, 50); //width should be the multiple of 8

    int y = 0;
    for (int i = 0; i < 35; i++) {
        
        if (i % 8 == 0) {
            // if new row or last icon
            paint.Clear(UNCOLORED);
        }
        
        int x = i % 8;
        renderIcon(paint, i + 1, 50 * x, 0);

        if (i % 8 == 7 || i == 34) {
            epd.SetPartialWindow(paint.GetImage(), 0, y * 50, paint.GetWidth(), paint.GetHeight());
            y++;
        }
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

    /* Reset initialized */
    initialized = false;

    Serial.println("Finished e-Paper");
}

Display::~Display() {
    if (initialized) {
        Serial.println("Warning: Destroying display, was still initialized");
        draw();
    }
}