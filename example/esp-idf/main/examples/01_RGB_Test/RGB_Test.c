#include "RGB_Test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

#define RGB_TEST_DELAY_MS        (1000)
#define RGB_TEST_BLOCK_LINES     (20)

#define RGB565_RED               (0xF800)
#define RGB565_GREEN             (0x07E0)
#define RGB565_BLUE              (0x001F)

static const char *TAG = "rgb_test";

static inline uint16_t rgb_test_rgb565_to_panel(uint16_t color)
{
#if (BSP_LCD_BITS_PER_PIXEL == 16) && !BSP_LCD_BIGENDIAN
    return (uint16_t)((color << 8) | (color >> 8));
#else
    return color;
#endif
}

static void rgb_test_fill_buffer(uint16_t *buffer, uint16_t color, size_t pixel_count)
{
    size_t index = 0;

    while (index < pixel_count) {
        buffer[index] = color;
        index++;
    }
}

static void rgb_test_draw_color(esp_lcd_panel_handle_t panel, uint16_t *buffer, uint16_t color)
{
    uint16_t line_count = RGB_TEST_BLOCK_LINES;
    uint16_t y = 0;
    uint16_t panel_color = rgb_test_rgb565_to_panel(color);

    rgb_test_fill_buffer(buffer, panel_color, BSP_LCD_H_RES * RGB_TEST_BLOCK_LINES);

    while (y < BSP_LCD_V_RES) {
        if ((BSP_LCD_V_RES - y) < RGB_TEST_BLOCK_LINES) {
            line_count = BSP_LCD_V_RES - y;
        } else {
            line_count = RGB_TEST_BLOCK_LINES;
        }

        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, y, BSP_LCD_H_RES, y + line_count, buffer));
        y += line_count;
    }
}

void rgb_test_run(void)
{
    static const uint16_t colors[] = {
        RGB565_RED,
        RGB565_GREEN,
        RGB565_BLUE,
    };
    static const char *color_names[] = {
        "red",
        "green",
        "blue",
    };
    bsp_display_cfg_t display_cfg = {0};
    esp_lcd_panel_handle_t panel = NULL;
    uint16_t *draw_buffer = NULL;
    size_t color_index = 0;

    ESP_ERROR_CHECK(bsp_display_new(&display_cfg, &panel, NULL));
    ESP_ERROR_CHECK(bsp_display_brightness_init());
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));

    draw_buffer = heap_caps_malloc(BSP_LCD_H_RES * RGB_TEST_BLOCK_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
    ESP_ERROR_CHECK(draw_buffer ? ESP_OK : ESP_ERR_NO_MEM);

    while (true) {
        ESP_LOGI(TAG, "show color: %s", color_names[color_index]);
        rgb_test_draw_color(panel, draw_buffer, colors[color_index]);
        vTaskDelay(pdMS_TO_TICKS(RGB_TEST_DELAY_MS));

        color_index++;
        if (color_index >= (sizeof(colors) / sizeof(colors[0]))) {
            color_index = 0;
        }
    }
}
