#include "RTC_Test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

#define RTC_TEST_DELAY_MS    (1000)

static const char *TAG = "rtc_test";

void rtc_test_run(void)
{
    char datetime_str[32];
    pcf85063a_datetime_t time = {
        .year = 2026,
        .month = 1,
        .day = 1,
        .dotw = 4,
        .hour = 12,
        .min = 0,
        .sec = 0,
    };

    ESP_ERROR_CHECK(bsp_rtc_init());
    ESP_ERROR_CHECK(bsp_set_rtc_time_date(time));

    while (true) {
        ESP_ERROR_CHECK(bsp_get_rtc_time_date(&time));
        ESP_ERROR_CHECK(bsp_datetime_to_str(datetime_str, sizeof(datetime_str), time));
        ESP_LOGI(TAG, "rtc time: %s", datetime_str);
        vTaskDelay(pdMS_TO_TICKS(RTC_TEST_DELAY_MS));
    }
}
