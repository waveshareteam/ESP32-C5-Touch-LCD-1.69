#include "Clock_App.h"
#include "Clock_Controller.h"
#include "Clock_Face_Classic.h"
#include "Clock_Face_Digital.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"

#define CLOCK_APP_IDLE_MS      (1000)
#define CLOCK_APP_UPDATE_MS    (200)
#define CLOCK_APP_FACE_ID      CLOCK_FACE_ID_CLASSIC

static const char *TAG = "clock_app";
static const clock_face_ops_t *clock_app_current_face_ops;

static const clock_face_ops_t *clock_app_get_face_ops(clock_face_id_t face_id)
{
    switch (face_id) {
    case CLOCK_FACE_ID_DIGITAL:
        return &clock_face_digital_ops;
    case CLOCK_FACE_ID_CLASSIC:
    default:
        return &clock_face_classic_ops;
    }
}

static void clock_app_timer_cb(lv_timer_t *timer)
{
    clock_view_data_t view_data = {0};

    LV_UNUSED(timer);

    if (clock_app_current_face_ops == NULL) {
        return;
    }

    clock_controller_get_view_data(&view_data);
    clock_app_current_face_ops->update(&view_data);
}

static void clock_app_create_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);

    clock_app_current_face_ops = clock_app_get_face_ops(CLOCK_APP_FACE_ID);
    clock_app_current_face_ops->create(screen);

    lv_scr_load(screen);
    lv_timer_create(clock_app_timer_cb, CLOCK_APP_UPDATE_MS, NULL);
    clock_app_timer_cb(NULL);
}

void clock_test_run(void)
{
    lv_display_t *display = NULL;

    ESP_ERROR_CHECK(bsp_i2c_init());
    ESP_ERROR_CHECK(bsp_rtc_init());
    ESP_ERROR_CHECK(bsp_display_set_partial_mode(true, 40));

    display = bsp_display_start();
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));
    ESP_ERROR_CHECK(bsp_display_lock(0));
    clock_app_create_screen();
    bsp_display_unlock();

    ESP_LOGI(TAG, "clock app started");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(CLOCK_APP_IDLE_MS));
    }
}
