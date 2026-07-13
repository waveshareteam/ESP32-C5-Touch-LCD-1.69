/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "Brookesia_Test.h"

#include <new>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "esp_brookesia.hpp"
#include "lvgl.h"

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Brookesia_Test"
#include "esp_lib_utils.h"

namespace {

static constexpr uint32_t BROOKESIA_TEST_IDLE_MS = 1000;

static lv_display_t *display = nullptr;
static ESP_Brookesia_Phone *phone = nullptr;

class brookesia_minimal_app_t : public ESP_Brookesia_PhoneApp {
public:
    brookesia_minimal_app_t()
        : ESP_Brookesia_PhoneApp("Hello", &esp_brookesia_image_small_app_launcher_default_98_98, true)
    {
    }

    ~brookesia_minimal_app_t() override = default;

protected:
    bool run(void) override
    {
        lv_obj_t *label = lv_label_create(lv_scr_act());

        ESP_UTILS_CHECK_NULL_RETURN(label, false, "Create app label failed");

        lv_obj_set_width(label, lv_pct(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_label_set_text(label, "Hello Brookesia\nMinimal App");
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);

        return true;
    }

    bool back(void) override
    {
        return notifyCoreClosed();
    }
};

static brookesia_minimal_app_t minimal_app;

bool init_bsp_display(void)
{
    esp_lv_adapter_config_t lv_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = lv_cfg,
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    lv_cfg.task_min_delay_ms = 1;
    lv_cfg.task_max_delay_ms = 5;
    cfg.lv_adapter_cfg = lv_cfg;

    ESP_UTILS_CHECK_ERROR_RETURN(bsp_i2c_init(), false, "Initialize I2C failed");

    display = bsp_display_start_with_config(&cfg);
    ESP_UTILS_CHECK_NULL_RETURN(display, false, "Start display failed");
    ESP_UTILS_CHECK_ERROR_RETURN(bsp_display_backlight_on(), false, "Turn on display backlight failed");

    return true;
}

bool phone_lvgl_lock(int timeout_ms)
{
    uint32_t wait_ms = (timeout_ms < 0) ? 0 : (uint32_t)timeout_ms;

    return bsp_display_lock(wait_ms) == ESP_OK;
}

void phone_lvgl_unlock(void)
{
    bsp_display_unlock();
}

bool install_phone_apps(void)
{
    int app_id = phone->installApp(&minimal_app);

    ESP_UTILS_CHECK_FALSE_RETURN((app_id >= 0), false, "Install minimal app failed");
    ESP_UTILS_LOGI("Installed minimal app id=%d", app_id);

    return true;
}

bool init_phone_system(void)
{
    ESP_Brookesia_PhoneStylesheet_t *stylesheet = nullptr;
    bool ok = false;

    ESP_UTILS_CHECK_ERROR_RETURN(bsp_display_lock(0), false, "Lock display failed");

    do {
        phone = new (std::nothrow) ESP_Brookesia_Phone(display);
        if (phone == nullptr) {
            ESP_UTILS_LOGE("Create phone failed");
            break;
        }

        stylesheet = new (std::nothrow) ESP_Brookesia_PhoneStylesheet_t(ESP_BROOKESIA_PHONE_DEFAULT_DARK_STYLESHEET());
        if (stylesheet == nullptr) {
            ESP_UTILS_LOGE("Create stylesheet failed");
            break;
        }

        stylesheet->core.manager.flags.enable_app_save_snapshot = 0;
        stylesheet->core.manager.app.max_running_num = 1;
        stylesheet->home.flags.enable_recents_screen = 0;
        stylesheet->home.app_launcher.data.table.default_num = 1;

        if (!phone->addStylesheet(stylesheet)) {
            ESP_UTILS_LOGE("Add stylesheet failed");
            break;
        }
        if (!phone->activateStylesheet(stylesheet)) {
            ESP_UTILS_LOGE("Activate stylesheet failed");
            break;
        }
        if (!phone->setTouchDevice(bsp_display_get_input_dev())) {
            ESP_UTILS_LOGE("Set touch device failed");
            break;
        }

        phone->registerLvLockCallback(phone_lvgl_lock, 0);
        phone->registerLvUnlockCallback(phone_lvgl_unlock);

        if (!phone->begin()) {
            ESP_UTILS_LOGE("Begin phone failed");
            break;
        }
        if (!install_phone_apps()) {
            break;
        }

        ok = true;
    } while (false);

    delete stylesheet;
    bsp_display_unlock();

    return ok;
}

} // namespace

void brookesia_test_run(void)
{
    ESP_UTILS_CHECK_FALSE_EXIT(init_bsp_display(), "BSP display init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(init_phone_system(), "Initialize Brookesia phone failed");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(BROOKESIA_TEST_IDLE_MS));
    }
}
