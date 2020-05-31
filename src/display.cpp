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

    icon_buffer = new unsigned char[56 * 48 / 8];
    paint_icon = new Paint(icon_buffer, 56, 48);
    paint_icon->Clear(UNCOLORED);

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

void Display::renderIcon(uint8_t icon_type, int x, int y, bool scale_2 = false)
{
    paint_icon->Clear(UNCOLORED);

    // paint_icon->DrawVerticalLine(0, 0, 48, COLORED);
    // paint_icon->DrawVerticalLine(49, 0, 48, COLORED);
    // paint_icon->DrawHorizontalLine(0, 0, 50, COLORED);
    // paint_icon->DrawHorizontalLine(0, 47, 50, COLORED);

    // Drawing area will be overwritten in places of icons (no need to clear)

    const unsigned char *basic = nullptr;
    const int *basic_info = nullptr;
    getTemplate(icon_type, &basic_info, &basic);

    const unsigned char *precipitation = nullptr;
    const unsigned char *precipitation_alpha = nullptr;
    const int *precipitation_info = nullptr;
    getPrecipitation(icon_type, &precipitation_info, &precipitation, &precipitation_alpha);

    icon_type = icon_type % 100;
    bool is_dark_cloud = ((icon_type >= 4 && icon_type <= 11) || (icon_type >= 13 && icon_type <= 25) || icon_type == 33 || icon_type == 34);
    if (is_dark_cloud) {
        // draw background pattern for cloud first - align cloud bottom right
        paint_icon->DrawBuffer(CLOUD_DARK, CLOUD_DARK_INFO, basic_info[2] + basic_info[0] - CLOUD_DARK_INFO[0], basic_info[3] + basic_info[1] - CLOUD_DARK_INFO[1], COLORED);
    }

    // Draw template according to type
    if (basic_info != nullptr && basic != nullptr) {
        paint_icon->DrawBuffer(basic, basic_info, 0, 0, COLORED);
    }

    if (precipitation_info != nullptr && precipitation != nullptr && precipitation_alpha != nullptr) {
        // align cloud bottom right
        paint_icon->DrawBufferAlpha(precipitation, precipitation_alpha, precipitation_info, basic_info[2] + basic_info[0] - CLOUD_DARK_INFO[0], 0, COLORED);
    }

    // special types - no impact on height so leave out for calculation
    if (icon_type == 26) {
        // draw high fog, no transparency
        paint_icon->DrawBufferOpaque(FOG_TOP, FOG_TOP_INFO, 0, 0, COLORED);
    } else if (icon_type == 27) {
        // draw low fog, no transparency
        paint_icon->DrawBufferOpaque(FOG_BOTTOM, FOG_BOTTOM_INFO, 0, 0, COLORED);
    } else if (icon_type == 28) {
        // fog - draw both
        paint_icon->DrawBuffer(FOG_TOP, FOG_TOP_INFO, 0, 0, COLORED);
        paint_icon->DrawBuffer(FOG_BOTTOM, FOG_BOTTOM_INFO, 0, 0, COLORED);
    }

    static unsigned char zero[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    int top_y = 0, bottom_y = 0;
    while(top_y < 48) {

        if (memcmp(zero, icon_buffer + top_y * 7 * sizeof(char), 7) != 0) {
            break;
        }
        top_y++;
    }
    while(bottom_y < 48) {
        if (memcmp(zero, icon_buffer + (47-bottom_y) * 7 * sizeof(char), 7) != 0) {
            break;
        }
        bottom_y++;
    }

    int offset_y = (bottom_y - top_y) / 2;

    int info[] = {56,48,0,0};

    if (scale_2) {
        paint->DrawBufferDouble(icon_buffer, info, x, y + offset_y, COLORED);
    } else {
        paint->DrawBuffer(icon_buffer, info, x, y + offset_y, COLORED);
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
        renderIcon(f.icon, x + 15, 206);

        int tempMin = roundf(f.tempMin);
        int tempMax = roundf(f.tempMax);

        const char *c_1 = String(tempMin, DEC).c_str();
        const char *c_2 = "|";

        int t_1 = getTextWidth(c_1, CONSOLAS, CONSOLAS_INFO);
        int t_2 = getTextWidth(c_2, CONSOLAS, CONSOLAS_INFO);

        int middle_x = x + 40 - (int)floorf((float) t_2 / 2) - t_1;
        renderText(middle_x, 262, String(String(tempMin, DEC) + "|" + String(tempMax, DEC)).c_str(), CONSOLAS, CONSOLAS_INFO);

        char *weekday = dayShortStr(f.weekDay);
        int day_width = getTextWidth(weekday, GOTHIC18, GOTHIC18_INFO);
        renderText(x + 40 - (int)floorf((float)day_width / 2), 288, weekday, GOTHIC18, GOTHIC18_INFO);
    }
}

void Display::renderPrecipitation(const Weather &weather, int start_hour, int offset_hour) {
    int hour, i_part = 0;
    float y_max = 1.f;

    // offset hour only plays a role if > 3 (last update older than 3 hours. should not happen)
    // in this case we just move everything forward 3h
    offset_hour = offset_hour / 3;

    int offset = 11;

    // precipitation 1h starts in the future at start_low. go forward skipping period (0 - 6AM)
    int offset_low_h = (weather.start_low - weather.start) / 3600;

    // 10MIN section
    int bar_width = 3;

    for (int i = offset_hour * 3; i < offset_low_h * 6 && i_part < (width - 2*offset); i++) {

        hour = start_hour + i / 6;
        hour = hour % 24;
        if (hour < HOUR_START) continue;

        if (weather.precipitation10min[i] > 0) {
            int x = offset + i_part;
            int y0 = 20.f * fminf(weather.precipitation10min[i] / y_max, y_max);
            paint->DrawFilledRectangle(x, 72 - y0, x + bar_width - 2, 72, COLORED);
        }

        i_part += bar_width;
    }

    // 1H section
    bar_width = 3 * 6;

    for (int i = offset_hour * 3; i < NUM_1H && i_part < (width - 2*offset); i++)
    {
        hour = (start_hour + offset_low_h + i) % 24;
        if (hour < HOUR_START) continue;

        if (weather.precipitation1h[i] > 0) {
            int x = offset + i_part;
            int y0 = 20.f * fminf(weather.precipitation1h[i] / y_max, y_max);
            paint->DrawFilledRectangle(x, 72 - y0, x + bar_width - 2, 72, COLORED);
        }

        i_part += bar_width;
    }

    
}

void Display::render24hIcons(const Weather &weather, int num_steps, int start_hour, int offset_hour)
{
    int start_i = offset_hour / 3;

    int part = 54;
    int offset = (width - 7 * part) / 2;

    for (int i = 0, i_part = 0; i < num_steps - start_i && i_part < 7; i++) {

        int hour = start_hour + i * 3;
        if (hour % 24 < HOUR_START) continue;

        String text = String(hour % 24) + "**";

        int x = offset + i_part * part;
        Display::renderText(x + 3, 78, text.c_str(), GOTHIC18, GOTHIC18_INFO);
        paint->DrawVerticalLine(x, 73, 4, COLORED);

        renderIcon(weather.icons[start_i + i], offset + i_part * part + part / 2 - 25, 2);
        i_part++;
    }

    renderPrecipitation(weather, start_hour, offset_hour);
}

void Display::renderTodayOverview(const WeatherForecast &forecast, time_t start)
{
    struct tm start_tm = *localtime(&start);
    String date(String(dayShortStr(start_tm.tm_wday + 1)) + ", " + String(start_tm.tm_mday) + ". " + String(monthNames[start_tm.tm_mon]));
    int date_width = getTextWidth(date.c_str(), GOTHIC18, GOTHIC18_INFO);
    renderText((int)fminf(width - 5 - date_width, 257), 180, date.c_str(), GOTHIC18, GOTHIC18_INFO);

    renderIcon(forecast.icon, 257, 120);

    int day_middle_y = 136;

    // Move up day temp if we have to show precipitation (keep 8px distance around middle)
    if (roundf(forecast.precipitation * 10) >= 1) {
        paint->DrawBuffer(PRECIPITATION, PRECIPITATION_INFO, 312, 151, COLORED);
        renderText(329, 148, String(String(forecast.precipitation, 1) + "mm").c_str(), CONSOLAS, CONSOLAS_INFO);
        day_middle_y -= 13;
    }
    int day_temp_max = ceilf(forecast.tempMax);
    int day_temp_min = roundf(forecast.tempMin);

    renderText(329, day_middle_y, String(String(day_temp_min) + "|" + String(day_temp_max)).c_str(), CONSOLAS, CONSOLAS_INFO);
}

void Display::renderCurrentWeather(const Weather &weather, int current_hour, int offset_hour)
{
    int offset_3h = offset_hour / 3;
    float current_temperature = weather.temperatures[offset_hour];
    uint8_t current_icon = weather.icons[offset_3h];

    String hour_temp(current_temperature, 1);

    int hour_temp_width = getTextWidth(hour_temp.c_str(), ROBOTO48_REGULAR, ROBOTO48_REGULAR_INFO);
    renderText(120, 128, String(hour_temp + "°").c_str(), ROBOTO48_REGULAR, ROBOTO48_REGULAR_INFO);
    
    renderIcon(current_icon, 10, 100, true);

    String hour(String(current_hour) + ":00");
    int hour_width = getTextWidth(hour.c_str(), GOTHIC18, GOTHIC18_INFO);

    // center font under hour temperature
    renderText(120 + (int)roundf((float)hour_temp_width / 2 - (float)hour_width / 2), 180, hour.c_str(), GOTHIC18, GOTHIC18_INFO);
}

void Display::renderWeather(Weather weather, int current_hour, int offset_hour)
{
    paint->DrawHorizontalLine(0, 72, width, COLORED);
    paint->DrawHorizontalLine(0, 200, width, COLORED);

    render24hIcons(weather, NUM_3H, current_hour - current_hour % 3, offset_hour);
    renderCurrentWeather(weather, current_hour, offset_hour);
    renderTodayOverview(weather.forecasts[0], weather.start);
    renderWeatherForecast(weather.forecasts, NUM_FORECASTS);

    if (offset_hour > 0) {
        paint->InvertRectangle(0, 72, 11 + offset_hour * 18, 93, COLORED);
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