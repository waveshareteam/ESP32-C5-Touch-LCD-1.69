#include <Arduino.h>
#include <esp32_c5_touch_lcd_1_69.h>
#include <LVGL_8.4.0/lvgl/lvgl.h>
#include <LVGL_8.4.0/lvgl/demos/benchmark/lv_demo_benchmark.h>

static const bsp_display_lvgl_full_frame_cfg_t lgvl_config = {
    .use_psram = true,
    .double_buffer = true,
    .full_refresh = true,
};

void setup(void)
{
    void *display = NULL;
    Serial.begin(115200);
    bsp_display_start_full_frame(&lgvl_config);
    bsp_display_brightness_set(100);

    bsp_display_lock(0);
    lv_demo_benchmark_set_max_speed(true);
    lv_demo_benchmark();
    bsp_display_unlock();
}

void loop(void)
{
    delay(1000);
}
