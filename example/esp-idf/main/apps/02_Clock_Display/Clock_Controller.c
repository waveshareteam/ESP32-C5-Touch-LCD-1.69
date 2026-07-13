#include "Clock_Controller.h"

#include <string.h>
#include <stdio.h>

#include "bsp/esp-bsp.h"

static void clock_controller_set_error_view_data(clock_view_data_t *view_data)
{
    memset(view_data, 0, sizeof(*view_data));
    view_data->rtc_valid = false;
    strlcpy(view_data->time_text, "RTC ERR", sizeof(view_data->time_text));
    strlcpy(view_data->date_text, "check rtc", sizeof(view_data->date_text));
}

esp_err_t clock_controller_get_view_data(clock_view_data_t *view_data)
{
    esp_err_t err = ESP_OK;
    pcf85063a_datetime_t time = {0};

    err = bsp_get_rtc_time_date(&time);
    if (err != ESP_OK) {
        clock_controller_set_error_view_data(view_data);
        return err;
    }

    memset(view_data, 0, sizeof(*view_data));
    view_data->rtc_valid = true;
    snprintf(view_data->time_text, sizeof(view_data->time_text), "%02u:%02u", time.hour, time.min);
    snprintf(view_data->date_text, sizeof(view_data->date_text), "%04u-%02u-%02u",
             time.year, time.month, time.day);

    view_data->sec_angle = (uint16_t)(time.sec * 60);
    view_data->min_angle = (uint16_t)(time.min * 60 + time.sec);
    view_data->hour_angle = (uint16_t)(((time.hour % 12) * 300) + (time.min * 5) + (time.sec / 12));

    return ESP_OK;
}
