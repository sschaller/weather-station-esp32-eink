#include "display.h"
#include "data.h"
#include "spline.h"

#define Y_CURVES 80
#define NUM_LINES 4

#define LABEL_OFFSET 35
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

    // create buffer
    buffer = new unsigned char[width * height / 8];
    paint = new Paint(buffer, width, height);
    paint->Clear(UNCOLORED);

    initialized = true;

    return true;
}

void Display::renderWeatherForecast(WeatherForecast *forecasts, int num_forecasts)
{
    if (!initialize(true)) {
        return;
    }

    int i;
    for (i = 0; i < num_forecasts && i < 5; i++) {
        int x = 80 * i;
        WeatherForecast f = forecasts[i];
        renderIcon(f.icon, x + 15, 200);

        String temp = String(String(f.tempMin, DEC) + "|" + String(f.tempMax, DEC));
        int len = temp.length() * 11;
        int start = (80 - len) / 2;
        if(f.tempMin < 0) start -= 3;

        paint->DrawStringAt(x + start, 252, temp.c_str(), &Font16, COLORED);
        paint->DrawStringAt(x + 15, 276, dayShortStr(f.weekDay), &Font24, COLORED);
    }
}

void Display::renderIcon(int icon_type, int x, int y)
{
    // Drawing area will be overwritten in places of icons (no need to clear)

    bool is_moon = icon_type > 100;
    icon_type = icon_type % 100; // TODO add moon

    bool is_sunny = (icon_type == 1 || icon_type == 26 || icon_type == 27);
    bool is_mostly_sunny = (icon_type == 2);
    bool is_mostly_cloudy = (icon_type == 3 || icon_type == 4 || (icon_type >= 6 && icon_type <= 13) || (icon_type >= 29 && icon_type <= 34));
    bool is_cloudy = (icon_type == 5 || (icon_type >= 14 && icon_type <= 25) || icon_type == 35);
    bool is_dark_cloud = ((icon_type >= 4 && icon_type <= 11) || (icon_type >= 13 && icon_type <= 25) || icon_type == 33 || icon_type == 34);    

    // calculate offset for different cloud versions
    int offset_cloud_x = is_cloudy ? 5 : 10;

    if (is_dark_cloud) {
        // draw background pattern for cloud first    
        paint->DrawBuffer(CLOUD_DARK, CLOUD_DARK_SIZE, x + offset_cloud_x, y + 11, COLORED);
    }

    // Draw template according to type
    if (is_moon && is_sunny) {
        paint->DrawBuffer(MOON, MOON_SIZE, x + 13, y + 8, COLORED);
    } else if (is_sunny) {
        paint->DrawBuffer(SUNNY, SUNNY_SIZE, x + 8, y + 8, COLORED);
    } else if (is_moon && is_mostly_sunny) {
        paint->DrawBuffer(MOSTLY_MOON, MOSTLY_MOON_SIZE, x + 13, y + 8, COLORED);
    } else if (is_mostly_sunny) {
        paint->DrawBuffer(MOSTLY_SUNNY, MOSTLY_SUNNY_SIZE, x + 4, y + 2, COLORED);
    } else if (is_moon && is_mostly_cloudy) {
        paint->DrawBuffer(MOON_CLOUDY, MOON_CLOUDY_SIZE, x + 5, y, COLORED);
    } else if (is_mostly_cloudy) {
        paint->DrawBuffer(MOSTLY_CLOUDY, MOSTLY_CLOUDY_SIZE, x, y, COLORED);
    } else if (is_cloudy) {
        paint->DrawBuffer(CLOUDY, CLOUDY_SIZE, x + 5, y + 11, COLORED);
    }

    // custom types
    if (icon_type == 26) {
        // draw high fog, no transparency
        paint->DrawBufferOpaque(FOG_TOP, FOG_TOP_SIZE, x + 5, y + 7, COLORED);
    } else if (icon_type == 27) {
        // draw low fog, no transparency
        paint->DrawBufferOpaque(FOG_BOTTOM, FOG_BOTTOM_SIZE, x + 5, y + 26, COLORED);
    } else if (icon_type == 28) {
        // fog - draw both
        paint->DrawBuffer(FOG_TOP, FOG_TOP_SIZE, x + 5, y + 12, COLORED);
        paint->DrawBuffer(FOG_BOTTOM, FOG_BOTTOM_SIZE, x + 5, y + 26, COLORED);
    } else if (icon_type == 12) {
        // 1 bolt
        paint->DrawBufferAlpha(BOLT_1, BOLT_1_ALPHA, BOLT_1_SIZE, x + offset_cloud_x + 15, y + 28, COLORED);
    } else if (icon_type == 13 || icon_type == 23) {
        // 1 bolt, 2 rain
        paint->DrawBufferAlpha(BOLT_1_RAIN_2, BOLT_1_RAIN_2_ALPHA, BOLT_1_RAIN_2_SIZE, x + offset_cloud_x + 9, y + 28, COLORED);
    } else if (icon_type == 24) {
        // 2 bolts, 2 rain
        paint->DrawBufferAlpha(BOLT_2_RAIN_2, BOLT_2_RAIN_2_ALPHA, BOLT_2_RAIN_2_SIZE, x + offset_cloud_x + 9, y + 28, COLORED);
    } else if (icon_type == 25) {
        // 2 bolts, 4 rain
        paint->DrawBufferAlpha(BOLT_2_RAIN_4, BOLT_2_RAIN_4_ALPHA, BOLT_2_RAIN_4_SIZE, x + offset_cloud_x + 3, y + 28, COLORED);
    } else if (icon_type == 6 || icon_type == 29) {
        // 1 rain
        paint->DrawBufferAlpha(RAIN_1, RAIN_1_ALPHA, RAIN_1_SIZE, x + offset_cloud_x + 15, y + 32, COLORED);
    } else if (icon_type == 9 || icon_type == 14 || icon_type == 32) {
        // 2 rain
        paint->DrawBufferAlpha(RAIN_2, RAIN_2_ALPHA, RAIN_2_SIZE, x + offset_cloud_x + 12, y + 32, COLORED);
    } else if (icon_type == 17 || icon_type == 33) {
        // 3 rain
        paint->DrawBufferAlpha(RAIN_3, RAIN_3_ALPHA, RAIN_3_SIZE, x + offset_cloud_x + 11, y + 31, COLORED);
    } else if (icon_type == 20) {
        // 4 rain
        paint->DrawBufferAlpha(RAIN_4, RAIN_4_ALPHA, RAIN_4_SIZE, x + offset_cloud_x + 7, y + 30, COLORED);
    } else if (icon_type == 8 || icon_type == 30) {
        // 1 snow
        paint->DrawBufferAlpha(SNOW_1, SNOW_1_ALPHA, SNOW_1_SIZE, x + offset_cloud_x + 14, y + 33, COLORED);
    } else if (icon_type == 11 || icon_type == 16) {
        // 2 snow
        paint->DrawBufferAlpha(SNOW_2, SNOW_2_ALPHA, SNOW_2_SIZE, x + offset_cloud_x + 11, y + 32, COLORED);
    } else if (icon_type == 19 || icon_type == 34) {
        // 3 snow
        paint->DrawBufferAlpha(SNOW_3, SNOW_3_ALPHA, SNOW_3_SIZE, x + offset_cloud_x + 11, y + 31, COLORED);
    } else if (icon_type == 22) {
        // 4 snow
        paint->DrawBufferAlpha(SNOW_4, SNOW_4_ALPHA, SNOW_4_SIZE, x + offset_cloud_x + 4, y + 33, COLORED);
    } else if (icon_type == 7 || icon_type == 15 || icon_type == 31) {
        // 1 rain 1 snow
        paint->DrawBufferAlpha(RAIN_1_SNOW_1, RAIN_1_SNOW_1_ALPHA, RAIN_1_SNOW_1_SIZE, x + offset_cloud_x + 12, y + 31, COLORED);
    } else if (icon_type == 10) {
        // 2 rain 1 snow
        paint->DrawBufferAlpha(RAIN_2_SNOW_1, RAIN_2_SNOW_1_ALPHA, RAIN_2_SNOW_1_SIZE, x + offset_cloud_x + 6, y + 31, COLORED);
    } else if (icon_type == 18) {
        // 2 rain 2 snow
        paint->DrawBufferAlpha(RAIN_2_SNOW_2, RAIN_2_SNOW_2_ALPHA, RAIN_2_SNOW_2_SIZE, x + offset_cloud_x + 7, y + 30, COLORED);
    } else if (icon_type == 21) {
        // 3 rain 3 snow
        paint->DrawBufferAlpha(RAIN_3_SNOW_3, RAIN_3_SNOW_3_ALPHA, RAIN_3_SNOW_3_SIZE, x + offset_cloud_x + 6, y + 31, COLORED);
    }
}

void Display::renderTemperatureCurve(float *temperatures, int num_entries, float y_min, float y_max) {
    float range = y_max - y_min;

    int limit_x = width - LABEL_OFFSET - 25;
    float step_size = static_cast<float>(limit_x) / (num_entries - 1);
    
    for (int i = 1; i < num_entries; i++) {
        int x1 = LABEL_OFFSET + (i - 1) * step_size;
        int x2 = LABEL_OFFSET + i * step_size;

        int y1 = 80.f * (y_max - temperatures[i - 1]) / range;
        int y2 = 80.f * (y_max - temperatures[i]) / range;

        paint->DrawLine(x1, Y_CURVES + y1, x2, Y_CURVES + y2, COLORED);
        paint->DrawLine(x1, Y_CURVES + y1 - 1, x2, Y_CURVES + y2 - 1, COLORED);
    }
}

void Display::renderTemperatureCurves(float *temperatures, float *precipitation, int num_points) {
    float y_min = 100.f;
    float y_max = -100.f;

    for (int i = 0; i < num_points; i++) {
        if(temperatures[i] < y_min) y_min = temperatures[i];
        if(temperatures[i] > y_max) y_max = temperatures[i];
    }

    float step = 5.f;

    float y_lower = step * floorf(y_min / step);
    float y_upper = step * ceilf(y_max / step);

    int steps = (y_upper - y_lower) / step;

    if (steps > NUM_LINES) {
        step = 10.f;

        y_lower = step * floorf(y_min / step);
        y_upper = step * ceilf(y_max / step);

        steps = (y_upper - y_lower) / step;
    }

    float rest = NUM_LINES - steps;

    int rest_upper = floorf(rest / 2);

    if (y_upper - y_max <= y_min - y_lower) {
        rest_upper = ceilf(rest / 2);
    }
    int rest_lower = rest - rest_upper;

    y_lower -= rest_lower * step;
    y_upper += rest_upper * step;

    for (int i = 0; i <= NUM_LINES; i++) {
        int y = Y_CURVES + (NUM_LINES - i) * 80.f / NUM_LINES;
        paint->DrawHorizontalLine(LABEL_OFFSET, y, width - LABEL_OFFSET - 25, COLORED);

        int y_text = static_cast<int>(i * step + y_lower);
        String text = String(y_text);
        paint->DrawStringAt(LABEL_OFFSET - 4 - text.length() * 11, y - 6, text.c_str(), &Font16, COLORED);
    }

    renderTemperatureCurve(temperatures, num_points, y_lower, y_upper);

    renderPrecipitation(precipitation, num_points);
}

void Display::renderPrecipitation(float *precipitation, int num_points) {
    float y_max = -100.f;
    for(int i = 0; i < num_points; i++) {
        if(precipitation[i] > y_max) y_max = precipitation[i];
    }

    float step = fmax(1.f, ceilf(y_max / NUM_LINES));
    
    float y_upper = step * NUM_LINES;
    float y_lower = 0.f;

    float range = y_upper - y_lower;

    float limit_x = width - LABEL_OFFSET - 25;
    int bar_x = (limit_x - (num_points-1) * 2) / num_points;

    for(int i = 0; i < num_points; i++) {
        if (precipitation[i] == 0) continue;

        int x = i * (bar_x + 2);
        int y0 = 80.f * (y_upper - precipitation[i]) / range;
        int y1 = 80;
        
        paint->DrawFilledRectangle(LABEL_OFFSET + x, Y_CURVES + y0, LABEL_OFFSET + x + bar_x, Y_CURVES + y1, COLORED);
    }

    for(int i = 0; i <= NUM_LINES; i++) {
        int y = Y_CURVES + (NUM_LINES - i) * 80.f / NUM_LINES;

        int y_text = static_cast<int>(i * step + y_lower);
        String text = String(y_text);
        paint->DrawStringAt(width - 25 + 4, y - 6, text.c_str(), &Font16, COLORED);
    }
}

void Display::render24hIcons(int *icons, int num_steps)
{
    int start = 6;
    int step = 3;

    int part = (width - LABEL_OFFSET - 25) / (num_steps - 1);

    for (int i = 0; i < num_steps; i++) {

        int hour = start + i * step;

        String text = String(hour % 12) + (hour % 24 >= 12 ? "PM" : "AM");

        int x = LABEL_OFFSET + i * part - text.length() * 14 / 2;

        renderIcon(icons[i], LABEL_OFFSET + i * part - 25, 28);
        paint->DrawStringAt(x, 4, text.c_str(), &Font20, COLORED);
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
    epd.DisplayFrame(buffer);

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

    delete[] buffer;
    delete paint;
}