#ifndef Display_h
#define Display_h

#include "epd4in2.h"
#include "epdpaint.h"

#include "weather.h"

typedef enum updateError {
    ENone,
    EConnection,
    ETime,
    EWeather
} UpdateError;

class Display {
    bool initialized;
    Epd epd;

    const int width = 400; //width should be the multiple of 8
    const int height = 300;

    unsigned char *buffer;
    Paint *paint;
    
    unsigned char *icon_buffer;
    Paint *paint_icon;

    public:
        ~Display();
        bool initialize(bool clear_buffer);
        void calculateResolution(float &y_lower, float &y_upper, float &step);
        void renderIcon(uint8_t icon_type, int x, int y, bool scale_2);
        void getTemplate(uint8_t icon_type, const int **info, const unsigned char **data);
        void getPrecipitation(uint8_t icon_type, const int **info, const unsigned char **data, const unsigned char **alpha);

        void renderWeatherForecast(const WeatherForecast *forecast, int num_forecasts);
        void renderPrecipitation(const Weather &weather, int start_hour, int offset_hour);
        void renderDailyGraph(float *temperatures, float *precipitation, int num_points, int current_hour);
        void render24hIcons(const Weather &weather, int num_steps, int start_hour, int offset_hour);
        void renderTodayOverview(const WeatherForecast &forecast, time_t start);
        void renderCurrentWeather(const Weather &weather, int current_hour, int offset_hour);
        void renderWeather(const Weather weather, int current_hour, int offset_hour);
        void renderText(int x, int y, const char *str, const unsigned char *font, const int *info);
        void renderError(UpdateError error);

        int getTextWidth(const char *str, const unsigned char *font, const int *info);
        int getLetterForFont(int letter, const unsigned char *font, const int *info);

        void print();
        void draw();

};

#endif /* Display_h */
