#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <esp32_c5_touch_lcd_1_69.h>
#include <PCF85063A.h>

static PCF85063A rtc(Wire);

static int month_from_build_date(const char *build_date)
{
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    int month = 0;

    while (month < 12) {
        if (strncmp(build_date, months[month], 3) == 0) {
            return month;
        }
        month++;
    }

    return 0;
}

static void rtc_set_to_build_time(void)
{
    struct tm now_tm = {};
    const char *build_date = __DATE__;
    const char *build_time = __TIME__;

    now_tm.tm_year = ((build_date[7] - '0') * 1000 + (build_date[8] - '0') * 100 +
                      (build_date[9] - '0') * 10 + (build_date[10] - '0')) - 1900;
    now_tm.tm_mon = month_from_build_date(build_date);
    now_tm.tm_mday = (build_date[4] == ' ') ? (build_date[5] - '0')
                                            : ((build_date[4] - '0') * 10 + (build_date[5] - '0'));
    now_tm.tm_hour = (build_time[0] - '0') * 10 + (build_time[1] - '0');
    now_tm.tm_min = (build_time[3] - '0') * 10 + (build_time[4] - '0');
    now_tm.tm_sec = (build_time[6] - '0') * 10 + (build_time[7] - '0');

    rtc.set(&now_tm);
}

void setup(void)
{
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    bsp_display_brightness_init();
    bsp_display_brightness_set(100);

    if (!bsp_i2c_init()) {
        Serial.println("I2C init failed");
        while (true) {
            delay(1000);
        }
    }

    rtc.begin();

    if (rtc.oscillator_stop()) {
        Serial.println("RTC lost power, set to build time");
        rtc_set_to_build_time();
    } else {
        Serial.println("RTC running");
    }
}

void loop(void)
{
    time_t current_time = rtc.time(NULL);
    struct tm *now_tm = localtime(&current_time);

    if (now_tm != NULL) {
        Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\r\n",
            now_tm->tm_year + 1900,
            now_tm->tm_mon + 1,
            now_tm->tm_mday,
            now_tm->tm_hour,
            now_tm->tm_min,
            now_tm->tm_sec);
    } else {
        Serial.println("RTC read failed");
    }

    delay(1000);
}
