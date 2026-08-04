#include "Honeycomb_Demo.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"

#define HONEYCOMB_ICON_COUNT            (25)
#define HONEYCOMB_COLUMN_COUNT          (5)
#define HONEYCOMB_ICON_SIZE             (30)
#define HONEYCOMB_ICON_STEP_X           (52)
#define HONEYCOMB_ICON_STEP_Y           (45)
#define HONEYCOMB_SCALE_MIN             (92)
#define HONEYCOMB_SCALE_MAX             (560)
#define HONEYCOMB_SCALE_RADIUS          (150)
#define HONEYCOMB_ICON_COLOR_COUNT      (10)
#define HONEYCOMB_TITLE_OFFSET_Y        (10)
#define HONEYCOMB_HINT_OFFSET_Y         (-14)

static const char *TAG = "honeycomb_demo";

static lv_obj_t *honeycomb_icons[HONEYCOMB_ICON_COUNT];
static lv_point_t honeycomb_base_center[HONEYCOMB_ICON_COUNT];
static lv_point_t honeycomb_offset;
static int32_t honeycomb_last_scale[HONEYCOMB_ICON_COUNT];
static lv_point_t honeycomb_last_pos[HONEYCOMB_ICON_COUNT];

static const uint32_t honeycomb_icon_color[] = {
    0x5B8FF9, 0x61DDAA, 0x65789B, 0xF6BD16, 0x7262FD,
    0x78D3F8, 0x9661BC, 0xF6903D, 0x008685, 0xF08BB4
};

static int32_t honeycomb_demo_calculate_scale(int32_t distance)
{
    int32_t radius = HONEYCOMB_SCALE_RADIUS;
    int32_t remain = 0;
    int32_t scale_range = HONEYCOMB_SCALE_MAX - HONEYCOMB_SCALE_MIN;

    if (distance >= radius) {
        return HONEYCOMB_SCALE_MIN;
    }

    remain = radius - distance;
    return HONEYCOMB_SCALE_MIN + ((remain * remain * scale_range) / (radius * radius));
}

static void honeycomb_demo_refresh_layout(void)
{
    lv_display_t *display = lv_display_get_default();
    int32_t screen_w = lv_display_get_horizontal_resolution(display);
    int32_t screen_h = lv_display_get_vertical_resolution(display);
    int32_t center_x = screen_w / 2;
    int32_t center_y = screen_h / 2;
    uint32_t index = 0;

    while (index < HONEYCOMB_ICON_COUNT) {
        int32_t icon_center_x = honeycomb_base_center[index].x + honeycomb_offset.x;
        int32_t icon_center_y = honeycomb_base_center[index].y + honeycomb_offset.y;
        int32_t dx = icon_center_x - center_x;
        int32_t dy = icon_center_y - center_y;
        int32_t distance_sq = dx * dx + dy * dy;
        int32_t distance = 0;
        int32_t scale = 0;
        int32_t icon_x = 0;
        int32_t icon_y = 0;
        lv_sqrt_res_t sqrt_result;

        lv_sqrt(distance_sq, &sqrt_result, 0xFFFF);
        distance = (int32_t)sqrt_result.i;
        scale = honeycomb_demo_calculate_scale(distance);
        icon_x = icon_center_x - (HONEYCOMB_ICON_SIZE / 2);
        icon_y = icon_center_y - (HONEYCOMB_ICON_SIZE / 2);

        if ((honeycomb_last_pos[index].x != icon_x) || (honeycomb_last_pos[index].y != icon_y)) {
            lv_obj_set_pos(honeycomb_icons[index], icon_x, icon_y);
            honeycomb_last_pos[index].x = icon_x;
            honeycomb_last_pos[index].y = icon_y;
        }

        if (honeycomb_last_scale[index] != scale) {
            lv_obj_set_style_transform_scale(honeycomb_icons[index], scale, LV_PART_MAIN | LV_STATE_DEFAULT);
            honeycomb_last_scale[index] = scale;
        }
        index++;
    }
}

static void honeycomb_demo_drag_event_cb(lv_event_t *event)
{
    lv_indev_t *indev = lv_indev_active();
    lv_point_t vector = {0};

    LV_UNUSED(event);

    if (indev == NULL) {
        return;
    }

    lv_indev_get_vect(indev, &vector);
    honeycomb_offset.x += vector.x;
    honeycomb_offset.y += vector.y;
    honeycomb_demo_refresh_layout();
}

static const char *honeycomb_demo_get_icon_text(uint32_t index)
{
    static const char *const icon_text[] = {
        "CLK", "MUS", "MAP", "MSG", "SET",
        "BAT", "CAL", "NFC", "REC", "WTH",
        "RUN", "PAY", "IMG", "MIC", "ALM",
        "GPS", "BLE", "APP", "OTA", "RTC",
        "PWM", "USB", "IMU", "WIFI", "UI"
    };

    return icon_text[index];
}

static void honeycomb_demo_create_icon(lv_obj_t *screen, uint32_t index)
{
    lv_obj_t *icon = lv_obj_create(screen);
    lv_obj_t *label = lv_label_create(icon);
    uint32_t color = honeycomb_icon_color[index % HONEYCOMB_ICON_COLOR_COUNT];

    lv_obj_remove_style_all(icon);
    lv_obj_set_size(icon, HONEYCOMB_ICON_SIZE, HONEYCOMB_ICON_SIZE);
    lv_obj_set_style_radius(icon, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(icon, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(icon, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(icon, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(icon, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_x(icon, HONEYCOMB_ICON_SIZE / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(icon, HONEYCOMB_ICON_SIZE / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_label_set_text(label, honeycomb_demo_get_icon_text(index));
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    honeycomb_icons[index] = icon;
    honeycomb_last_scale[index] = -1;
    honeycomb_last_pos[index].x = INT32_MIN;
    honeycomb_last_pos[index].y = INT32_MIN;
}

static void honeycomb_demo_create_icons(lv_obj_t *screen)
{
    lv_display_t *display = lv_display_get_default();
    int32_t screen_center_x = lv_display_get_horizontal_resolution(display) / 2;
    int32_t screen_center_y = lv_display_get_vertical_resolution(display) / 2;
    int32_t center_index = HONEYCOMB_ICON_COUNT / 2;
    int32_t center_row = center_index / HONEYCOMB_COLUMN_COUNT;
    int32_t center_column = center_index % HONEYCOMB_COLUMN_COUNT;
    int32_t center_row_offset_x = (center_row & 0x01) ? (HONEYCOMB_ICON_STEP_X / 2) : 0;
    uint32_t index = 0;

    while (index < HONEYCOMB_ICON_COUNT) {
        int32_t row = (int32_t)(index / HONEYCOMB_COLUMN_COUNT);
        int32_t column = (int32_t)(index % HONEYCOMB_COLUMN_COUNT);
        int32_t row_offset_x = (row & 0x01) ? (HONEYCOMB_ICON_STEP_X / 2) : 0;
        int32_t relative_column = column - center_column;
        int32_t relative_row = row - center_row;

        honeycomb_base_center[index].x = screen_center_x
                                       + (relative_column * HONEYCOMB_ICON_STEP_X)
                                       + (row_offset_x - center_row_offset_x);
        honeycomb_base_center[index].y = screen_center_y + (relative_row * HONEYCOMB_ICON_STEP_Y);
        honeycomb_demo_create_icon(screen, index);
        index++;
    }
}

static void honeycomb_demo_create_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *title = lv_label_create(screen);
    lv_obj_t *hint = lv_label_create(screen);

    honeycomb_offset.x = 0;
    honeycomb_offset.y = 0;

    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x081225), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(screen, honeycomb_demo_drag_event_cb, LV_EVENT_PRESSING, NULL);

    lv_label_set_text(title, "Honeycomb Demo");
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, HONEYCOMB_TITLE_OFFSET_Y);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE9F1FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(title, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_label_set_text(hint, "Drag icons");
    lv_obj_set_align(hint, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(hint, HONEYCOMB_HINT_OFFSET_Y);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x7B8AA8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(hint, LV_OBJ_FLAG_IGNORE_LAYOUT);

    honeycomb_demo_create_icons(screen);
    honeycomb_demo_refresh_layout();
    lv_scr_load(screen);
}

void honeycomb_demo_run(void)
{
    lv_display_t *display = NULL;


    display = bsp_display_start();
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));
    ESP_ERROR_CHECK(bsp_display_lock(0));
    honeycomb_demo_create_screen();
    bsp_display_unlock();

    ESP_LOGI(TAG, "honeycomb demo started");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
