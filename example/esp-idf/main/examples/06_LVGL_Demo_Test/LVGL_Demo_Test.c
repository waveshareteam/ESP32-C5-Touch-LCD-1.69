#include "LVGL_Demo_Test.h"
#include "benchmark/lv_demo_benchmark.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "demos/lv_demos.h"

#define LVGL_TEST_DELAY_MS    1000

static const char *TAG = "lvgl_test";

void lvgl_test_run(void)
{
    lv_display_t *display = NULL;

    display = bsp_display_start();
    
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_FAIL);

    ESP_ERROR_CHECK(bsp_display_brightness_set(100));

    ESP_ERROR_CHECK(bsp_display_lock(0));

    /* Running Widgets Demo */
    lv_demo_widgets();
    bsp_display_unlock();

    ESP_LOGI(TAG, "LVGL demo started");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(LVGL_TEST_DELAY_MS));
    }
}