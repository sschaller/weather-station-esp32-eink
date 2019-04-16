#include "display.h"
#include "data.h"

#define Y_CURVES 100
#define NUM_LINES 4

#define LABEL_OFFSET 35
#define COLORED     0
#define UNCOLORED   1

static char buffer[2+1];
char* dayShortStr(uint8_t day) {
    uint8_t index = day*2;
    for (int i=0; i < 2; i++)
        buffer[i] = pgm_read_byte(&(dayShortNames_P[index + i]));  
    buffer[2] = 0;
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

void Display::calculateResolution(float &y_lower, float &y_upper, float &step) {
    // Default step resolution
    step = 5.f;

    float y_min = y_lower;
    float y_max = y_upper;

    y_lower = step * floorf(y_min / step);
    y_upper = step * ceilf(y_max / step);

    int steps = (y_upper - y_lower) / step;

    // Too many steps, increase step resolution (5 > 10)
    if (steps > NUM_LINES) {
        step = 10.f;

        y_lower = step * floorf(y_min / step);
        y_upper = step * ceilf(y_max / step);

        steps = (y_upper - y_lower) / step;
    }

    float rest = NUM_LINES - steps;

    if (rest > 0) {
        // where to increase space first (where less space)
        int rest_upper = floorf(rest / 2);
        if (y_upper - y_max <= y_min - y_lower) {
            rest_upper = ceilf(rest / 2);
        }
        int rest_lower = rest - rest_upper;

        y_lower -= rest_lower * step;
        y_upper += rest_upper * step;
    }
}

void Display::renderIcon(uint8_t icon_type, int x, int y)
{
    // Drawing area will be overwritten in places of icons (no need to clear)

    int x_lowest = 50, x_highest = 0, y_lowest = 48, y_highest = 0;

    const unsigned char *basic = nullptr;
    const int *basic_info = nullptr;
    getTemplate(icon_type, &basic_info, &basic);

    const unsigned char *precipitation = nullptr;
    const unsigned char *precipitation_alpha = nullptr;
    const int *precipitation_info = nullptr;
    getPrecipitation(icon_type, &precipitation_info, &precipitation, &precipitation_alpha);

    if (basic_info != nullptr) {
        if (basic_info[2] < x_lowest) x_lowest = basic_info[2];
        if (basic_info[3] < y_lowest) y_lowest = basic_info[3];
        if (basic_info[2] + basic_info[0] > x_highest) x_highest = basic_info[2] + basic_info[0];
        if (basic_info[3] + basic_info[1] > y_highest) y_highest = basic_info[3] + basic_info[1];
    }

    if (precipitation_info != nullptr) {
        if (precipitation_info[2] < x_lowest) x_lowest = basic_info[2];
        if (precipitation_info[3] < y_lowest) y_lowest = basic_info[3];
        if (precipitation_info[2] + precipitation_info[0] > x_highest) x_highest = precipitation_info[2] + precipitation_info[0];
        if (precipitation_info[3] + precipitation_info[1] > y_highest) y_highest = precipitation_info[3] + precipitation_info[1];
    }

    int offset_x = (int)ceilf((50.f - (x_highest - x_lowest)) / 2) - x_lowest;
    int offset_y = (int)ceilf((48.f - (y_highest - y_lowest)) / 2) - y_lowest;

    x += offset_x;
    y += offset_y;

    icon_type = icon_type % 100;
    bool is_dark_cloud = ((icon_type >= 4 && icon_type <= 11) || (icon_type >= 13 && icon_type <= 25) || icon_type == 33 || icon_type == 34);
    if (is_dark_cloud) {
        // draw background pattern for cloud first - align cloud bottom right
        paint->DrawBuffer(CLOUD_DARK, CLOUD_DARK_INFO, x + basic_info[2] + basic_info[0] - CLOUD_DARK_INFO[0], y + basic_info[3] + basic_info[1] - CLOUD_DARK_INFO[1], COLORED);
    }

    // Draw template according to type
    if (basic_info != nullptr && basic != nullptr) {
        paint->DrawBuffer(basic, basic_info, x, y, COLORED);
    }

    if (precipitation_info != nullptr && precipitation != nullptr && precipitation_alpha != nullptr) {
        // align cloud bottom right
        paint->DrawBufferAlpha(precipitation, precipitation_alpha, precipitation_info, x + basic_info[2] + basic_info[0] - CLOUD_DARK_INFO[0], y, COLORED);
    }

    // special types - no impact on height so leave out for calculation
    if (icon_type == 26) {
        // draw high fog, no transparency
        paint->DrawBufferOpaque(FOG_TOP, FOG_TOP_INFO, x, y, COLORED);
    } else if (icon_type == 27) {
        // draw low fog, no transparency
        paint->DrawBufferOpaque(FOG_BOTTOM, FOG_BOTTOM_INFO, x, y, COLORED);
    } else if (icon_type == 28) {
        // fog - draw both
        paint->DrawBuffer(FOG_TOP, FOG_TOP_INFO, x, y, COLORED);
        paint->DrawBuffer(FOG_BOTTOM, FOG_BOTTOM_INFO, x, y, COLORED);
    }
}

void Display::getTemplate(uint8_t icon_type, const int **info, const unsigned char **data) {

    bool is_moon = icon_type > 100;
    icon_type = icon_type % 100;

    bool is_sunny = (icon_type == 1 || icon_type == 26 || icon_type == 27 || icon_type == 28);
    bool is_mostly_sunny = (icon_type == 2);
    bool is_mostly_cloudy = (icon_type == 3 || icon_type == 4 || (icon_type >= 6 && icon_type <= 13) || (icon_type >= 29 && icon_type <= 34));
    bool is_cloudy = (icon_type == 5 || (icon_type >= 14 && icon_type <= 25) || icon_type == 35);

    if (is_moon && is_sunny) {
        *info = &MOON_INFO[0];
        *data = &MOON[0];
    } else if (is_sunny) {
        *info = &SUNNY_INFO[0];
        *data = &SUNNY[0];
    } else if (is_moon && is_mostly_sunny) {
        *info = &MOSTLY_MOON_INFO[0];
        *data = &MOSTLY_MOON[0];
    } else if (is_mostly_sunny) {
        *info = &MOSTLY_SUNNY_INFO[0];
        *data = &MOSTLY_SUNNY[0];
    } else if (is_moon && is_mostly_cloudy) {
        *info = &MOON_CLOUDY_INFO[0];
        *data = &MOON_CLOUDY[0];
    } else if (is_mostly_cloudy) {
        *info = &MOSTLY_CLOUDY_INFO[0];
        *data = &MOSTLY_CLOUDY[0];
    } else if (is_cloudy) {
        *info = &CLOUDY_INFO[0];
        *data = &CLOUDY[0];
    }
}

void Display::getPrecipitation(uint8_t icon_type, const int **info, const unsigned char **data, const unsigned char **alpha) {

    icon_type = icon_type % 100;
    
    if (icon_type == 12) {
        // 1 bolt
        *info = &BOLT_1_INFO[0];
        *data = &BOLT_1[0];
        *alpha = &BOLT_1_ALPHA[0];
    } else if (icon_type == 13 || icon_type == 23) {
        // 1 bolt, 2 rain
        *info = &BOLT_1_RAIN_2_INFO[0];
        *data = &BOLT_1_RAIN_2[0];
        *alpha = &BOLT_1_RAIN_2_ALPHA[0];
    } else if (icon_type == 24) {
        // 2 bolts, 2 rain
        *info = &BOLT_2_RAIN_2_INFO[0];
        *data = &BOLT_2_RAIN_2[0];
        *alpha = &BOLT_2_RAIN_2_ALPHA[0];
    } else if (icon_type == 25) {
        // 2 bolts, 4 rain
        *info = &BOLT_2_RAIN_4_INFO[0];
        *data = &BOLT_2_RAIN_4[0];
        *alpha = &BOLT_2_RAIN_4_ALPHA[0];
    } else if (icon_type == 6 || icon_type == 29) {
        // 1 rain
        *info = &RAIN_1_INFO[0];
        *data = &RAIN_1[0];
        *alpha = &RAIN_1_ALPHA[0];
    } else if (icon_type == 9 || icon_type == 14 || icon_type == 32) {
        // 2 rain
        *info = &RAIN_2_INFO[0];
        *data = &RAIN_2[0];
        *alpha = &RAIN_2_ALPHA[0];
    } else if (icon_type == 17 || icon_type == 33) {
        // 3 rain
        *info = &RAIN_3_INFO[0];
        *data = &RAIN_3[0];
        *alpha = &RAIN_3_ALPHA[0];
    } else if (icon_type == 20) {
        // 4 rain
        *info = &RAIN_4_INFO[0];
        *data = &RAIN_4[0];
        *alpha = &RAIN_4_ALPHA[0];
    } else if (icon_type == 8 || icon_type == 30) {
        // 1 snow
        *info = &SNOW_1_INFO[0];
        *data = &SNOW_1[0];
        *alpha = &SNOW_1_ALPHA[0];
    } else if (icon_type == 11 || icon_type == 16) {
        // 2 snow
        *info = &SNOW_2_INFO[0];
        *data = &SNOW_2[0];
        *alpha = &SNOW_2_ALPHA[0];
    } else if (icon_type == 19 || icon_type == 34) {
        // 3 snow
        *info = &SNOW_3_INFO[0];
        *data = &SNOW_3[0];
        *alpha = &SNOW_3_ALPHA[0];
    } else if (icon_type == 22) {
        // 4 snow
        *info = &SNOW_4_INFO[0];
        *data = &SNOW_4[0];
        *alpha = &SNOW_4_ALPHA[0];
    } else if (icon_type == 7 || icon_type == 15 || icon_type == 31) {
        // 1 rain 1 snow
        *info = &RAIN_1_SNOW_1_INFO[0];
        *data = &RAIN_1_SNOW_1[0];
        *alpha = &RAIN_1_SNOW_1_ALPHA[0];
    } else if (icon_type == 10) {
        // 2 rain 1 snow
        *info = &RAIN_2_SNOW_1_INFO[0];
        *data = &RAIN_2_SNOW_1[0];
        *alpha = &RAIN_2_SNOW_1_ALPHA[0];
    } else if (icon_type == 18) {
        // 2 rain 2 snow
        *info = &RAIN_2_SNOW_2_INFO[0];
        *data = &RAIN_2_SNOW_2[0];
        *alpha = &RAIN_2_SNOW_2_ALPHA[0];
    } else if (icon_type == 21) {
        // 3 rain 3 snow
        *info = &RAIN_3_SNOW_3_INFO[0];
        *data = &RAIN_3_SNOW_3[0];
        *alpha = &RAIN_3_SNOW_3_ALPHA[0];
    }
}

void Display::renderWeatherForecast(const WeatherForecast *forecasts, int num_forecasts)
{
    for (int i = 0; i < num_forecasts - 1; i++) {
        int x = 80 * i;
        WeatherForecast f = forecasts[i + 1];
        renderIcon(f.icon, x + 15, 200);

        int tempMin = roundf(f.tempMin);
        int tempMax = roundf(f.tempMax);

        const char *c_1 = String(tempMin, DEC).c_str();
        const char *c_2 = "|";

        Serial.println(tempMin);

        int t_1 = getTextWidth(c_1, CONSOLAS, CONSOLAS_INFO);
        int t_2 = getTextWidth(c_2, CONSOLAS, CONSOLAS_INFO);
        // int t_3 = getTextWidth(c_3, CONSOLAS, CONSOLAS_INFO);

        int middle_x = x + 40 - (int)floorf((float) t_2 / 2) - t_1;
        renderText(middle_x, 252, String(String(tempMin, DEC) + "|" + String(tempMax, DEC)).c_str(), CONSOLAS, CONSOLAS_INFO);

        char *weekday = dayShortStr(f.weekDay);
        int day_width = getTextWidth(weekday, GOTHIC18, GOTHIC18_INFO);
        renderText(x + 40 - (int)floorf((float)day_width / 2), 283, weekday, GOTHIC18, GOTHIC18_INFO);
    }
}

void Display::renderTemperatureCurve(float *temperatures, int num_entries) {
    
    float y_min = 100.f;
    float y_max = -100.f;
    for (int i = 0; i < num_entries; i++) {
        if(temperatures[i] < y_min) y_min = temperatures[i];
        if(temperatures[i] > y_max) y_max = temperatures[i];
    }

    float step;
    calculateResolution(y_min, y_max, step);
    
    float range = y_max - y_min;

    for (int i = 0; i <= NUM_LINES; i++) {
        int y = Y_CURVES + (NUM_LINES - i) * 80.f / NUM_LINES;

        int y_text = static_cast<int>(i * step + y_min);
        String text = String(y_text);
        paint->DrawStringAt(LABEL_OFFSET - 4 - text.length() * 11, y - 6, text.c_str(), &Font16, COLORED);
    }

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

void Display::renderDailyGraph(float *temperatures, float *precipitation, int num_points, int current_hour) {
    for (int i = 0; i <= NUM_LINES; i++) {
        int y = Y_CURVES + (NUM_LINES - i) * 80.f / NUM_LINES;
        paint->DrawHorizontalLine(LABEL_OFFSET, y, width - LABEL_OFFSET - 25, COLORED);
    }
    renderTemperatureCurve(temperatures, num_points);
    renderPrecipitation(precipitation, num_points);

    if (current_hour >= HOUR_START) {
        float limit_x = width - LABEL_OFFSET - 25;
        float step_size = limit_x / (num_points - 1);
        paint->DrawVerticalLine(LABEL_OFFSET + (current_hour - HOUR_START) * step_size, Y_CURVES - 20, 80 + 20, COLORED);
        paint->DrawVerticalLine(LABEL_OFFSET + (current_hour - HOUR_START) * step_size + 1, Y_CURVES - 20, 80 + 20, COLORED);
    }
}

void Display::render24hIcons(uint8_t *icons, int num_steps)
{
    int start = 6;
    int step = 3;

    int part = (width - LABEL_OFFSET - 25) / (num_steps - 1);

    for (int i = 0; i < num_steps; i++) {

        int hour = start + i * step;

        String text = String(hour) + "H";

        int x = LABEL_OFFSET + i * part - text.length() * 14 / 2;

        renderIcon(icons[i], LABEL_OFFSET + i * part - 25, 28);
        paint->DrawStringAt(x, 4, text.c_str(), &Font20, COLORED);
    }
}

void Display::renderWeather(Weather weather, int current_hour)
{
    renderWeatherForecast(weather.forecasts, NUM_FORECASTS);
    // renderDailyGraph(weather.temperatures, weather.precipitation, NUM_1H, current_hour);
    render24hIcons(weather.icons, NUM_3H);

    paint->DrawHorizontalLine(0, 86, width, COLORED);
    paint->DrawHorizontalLine(0, 189, width, COLORED);
    // paint->DrawVerticalLine(30, 86, 189 - 86, COLORED);
    // paint->DrawVerticalLine(width - 31, 86, 189 - 86, COLORED);

    String hour_temp(weather.current.temperature, 1);

    int hour_temp_width = getTextWidth(hour_temp.c_str(), ROBOTO48_REGULAR, ROBOTO48_REGULAR_INFO);
    renderText(120, 115, String(hour_temp + "°").c_str(), ROBOTO48_REGULAR, ROBOTO48_REGULAR_INFO);
    
    renderLargeIcon(weather.current.icon, 10, 89);

    time_t current_weather_time = static_cast<time_t>(weather.current.time);
    struct tm current_weather_tm = *localtime(&current_weather_time);

    String date(String(dayShortStr(current_weather_tm.tm_wday + 1)) + ", " + String(current_weather_tm.tm_mday) + ". " + String(monthNames[current_weather_tm.tm_mon]));

    int date_width = getTextWidth(date.c_str(), GOTHIC18, GOTHIC18_INFO);
    renderText((int)fminf(width - 5 - date_width, 257), 164, date.c_str(), GOTHIC18, GOTHIC18_INFO);

    String hour(String(current_weather_tm.tm_hour) + ":00");
    int hour_width = getTextWidth(hour.c_str(), GOTHIC18, GOTHIC18_INFO);

    // center font under hour temperature
    renderText(120 + (int)roundf((float)hour_temp_width / 2 - (float)hour_width / 2), 164 + 2, hour.c_str(), GOTHIC18, GOTHIC18_INFO);

    // Day Weather
    WeatherForecast current_forecast = weather.forecasts[0];

    renderIcon(current_forecast.icon, 257, 107);

    int day_middle_y = 131;

    // Move up day temp if we have to show precipitation (keep 8px distance around middle)
    if (roundf(current_forecast.precipitation * 10) >= 1) {
        paint->DrawBuffer(PRECIPITATION, PRECIPITATION_INFO, 322, 140, COLORED);
        renderText(341, 136, String(String(current_forecast.precipitation, 1) + "mm").c_str(), CONSOLAS, CONSOLAS_INFO);
        day_middle_y -= 10;
    }
    int day_temp_max = ceilf(current_forecast.tempMax);
    int day_temp_min = roundf(current_forecast.tempMin);

    renderText(329, day_middle_y - 9, String(String(day_temp_min) + "|" + String(day_temp_max)).c_str(), CONSOLAS, CONSOLAS_INFO);

    if (current_hour >= 6 || current_hour == 0) {
        int x = 35.f + 330.f * ((float)current_hour - 6)/ 18;
        paint->DrawArrowUp(x, 86, 11, COLORED);
    }
}

void Display::renderLargeIcon(uint8_t icon_type, int x, int y) {
    int temp_x = 290, temp_y = 96;

    // Create temporary icon to copy and scale
    renderIcon(icon_type, temp_x, temp_y);
    
    // copy and scale icon + delete origin
    int i, j, p;
    for (j = 0; j < 48; j++) { // icon height
        for(i = 0; i < 50; i++) { // icon width
            p = (temp_y + j) * width + i + temp_x;
            if ((buffer[p / 8] & (0x80 >> (p % 8))) == 0) {
                paint->DrawPixel(x + i * 2, y + j * 2, COLORED);
                paint->DrawPixel(x + i * 2 + 1, y + j * 2, COLORED);
                paint->DrawPixel(x + i * 2, y + j * 2 + 1, COLORED);
                paint->DrawPixel(x + i * 2 + 1, y + j * 2 + 1, COLORED);
                paint->DrawPixel(temp_x + i, temp_y + j, UNCOLORED);
            }
        }
    }
}

void Display::renderError(UpdateError error) {
    if (error == UpdateError::ETime) {
        paint->DrawBuffer(NOTIME, NOTIME_INFO, 5, 137 - 10, COLORED);
    } else if (error == UpdateError::EConnection) {
        paint->DrawBuffer(NOCONNECTION, NOCONNECTION_INFO, 5, 137 - 10, COLORED);
    } else if (error == UpdateError::EWeather) {
        paint->DrawBuffer(NOWEATHER, NOWEATHER_INFO, 5, 137 - 10, COLORED);
    }
}

void Display::renderTime(time_t t, int x, int y) {
    struct tm *info = localtime (&t);
    char buffer[6];
    strftime (buffer,6,"%I:%M", info);

    paint->DrawStringAt(x, y, buffer, &Font12, COLORED);
}

void Display::renderText(int x, int y, const char *str, const unsigned char *font, const int *info) {
    int width = info[1], height = info[2];
    
    for(int i = 0; i < strlen(str); i++) {
        int found = getLetterForFont((int)str[i], font, info);
        if (found < 0) continue;

        int s_x = info[3 + found * 4 + 1];
        int w = info[3 + found * 4 + 2];
        int spacing = info[3 + found * 4 + 3];

        paint->DrawBufferLimited(font, width, s_x, 0, w, height, x, y, COLORED);
        x += w + spacing;
    }
}

int Display::getTextWidth(const char *str, const unsigned char *font, const int *info) {
    int x = 0;
    for(int i = 0; i < strlen(str); i++) {
        int found = getLetterForFont((int)str[i], font, info);
        if (found < 0) continue;

        int w = info[3 + found * 4 + 2];
        int spacing = info[3 + found * 4 + 3];

        x += w + spacing;
    }
    return x;
}

int Display::getLetterForFont(int letter, const unsigned char *font, const int *info) {
    for(int j = 0; j < info[0]; j++) {
        if (info[j * 4 + 3] == letter) {
            return j;
        }
    }
    return -1;
}

void Display::print()
{
    for (int i = 0; i < width * height / 8; i++) {
        Serial.print((int)buffer[i]);
        Serial.print(" ");
        delay(1);
    }
    
    Serial.println("");
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