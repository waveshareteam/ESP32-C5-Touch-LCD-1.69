#include <esp32_c5_touch_lcd_1_69.h>

static const uint16_t Colors[] = {
    0xF800,
    0x07E0,
    0x001F,
    0xFFFF,
};

static uint8_t color_index = 0;

void setup(void)
{
    Serial.begin(115200);
    delay(200);

    if (!bsp_display_init()) {
        Serial.println("bsp_display_init failed");
        while (true) {
            delay(1000);
        }
    }

    bsp_display_backlight_on();
    bsp_display_brightness_set(100);
}

void loop(void)
{
    bsp_display_fill(Colors[color_index]);
    Serial.printf("show color index: %u\n", color_index);

    color_index++;
    if (color_index >= (sizeof(Colors) / sizeof(Colors[0]))) {
        color_index = 0;
    }

    delay(1000);
}
