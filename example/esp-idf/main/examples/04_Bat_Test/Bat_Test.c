#include "Bat_Test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

#define BAT_TEST_DELAY_MS     (1000)

static const char *TAG = "bat_test";

void bat_test_run(uint16_t battery_capacity_mah)
{
    bsp_bat_info_t bat_info = {0};
    esp_err_t ret = ESP_OK;
    const char *battery_state = "Idle";
    const char *battery_note = "";
    lv_display_t *display = NULL;
    lv_obj_t *label = NULL;

    display = bsp_display_start();
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));
    ESP_ERROR_CHECK(bsp_bat_init(battery_capacity_mah));
    ESP_ERROR_CHECK(bsp_display_lock(0));

    label = lv_label_create(lv_screen_active());
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

    bsp_display_unlock();

    while (true) {
        ret = bsp_get_bat_info(&bat_info);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "battery info update failed: %s", esp_err_to_name(ret));
            ESP_ERROR_CHECK(bsp_display_lock(0));
            lv_label_set_text(label, "Battery Test\nBattery status updating");
            bsp_display_unlock();
            vTaskDelay(pdMS_TO_TICKS(BAT_TEST_DELAY_MS));
            continue;
        }
        battery_state = (bat_info.ma > 0) ? "Charging" : ((bat_info.ma < 0) ? "Discharging" : "Idle");
        battery_note = (bat_info.ma == 0) ? "battery not connected or full" : "battery connected";
        ESP_LOGI(TAG, "state=%s voltage=%u current=%d soc=%u temp=%u rem=%u fcc=%u",
            battery_state,
            bat_info.mv,
            bat_info.ma,
            bat_info.soc,
            bat_info.tc,
            bat_info.mah_rem,
            bat_info.mah_fcc);
        if (bat_info.ma == 0) {
            ESP_LOGW(TAG, "%s", battery_note);
        }
        ESP_ERROR_CHECK(bsp_display_lock(0));
        lv_label_set_text_fmt(label,
            "Battery Test\n"
            "State: %s\n"
            "Voltage: %u mV\n"
            "Current: %d mA\n"
            "SOC: %u %%\n"
            "Temp: %u C\n"
            "Capacity: %u mAh\n"
            "%s",
            battery_state,
            bat_info.mv,
            bat_info.ma,
            bat_info.soc,
            bat_info.tc,
            battery_capacity_mah,
            battery_note);
        bsp_display_unlock();
        vTaskDelay(pdMS_TO_TICKS(BAT_TEST_DELAY_MS));
    }
}
