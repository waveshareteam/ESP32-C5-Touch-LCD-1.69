#include "Clock_Face_Classic.h"

#include "lvgl.h"

#define CLOCK_FACE_CLASSIC_HOUR_PIVOT_X    (9)
#define CLOCK_FACE_CLASSIC_HOUR_PIVOT_Y    (66)
#define CLOCK_FACE_CLASSIC_MIN_PIVOT_X     (6)
#define CLOCK_FACE_CLASSIC_MIN_PIVOT_Y     (90)
#define CLOCK_FACE_CLASSIC_SEC_PIVOT_X     (4)
#define CLOCK_FACE_CLASSIC_SEC_PIVOT_Y     (98)

#define CLOCK_FACE_CLASSIC_CENTER_X        (120)
#define CLOCK_FACE_CLASSIC_CENTER_Y        (140)
#define CLOCK_FACE_CLASSIC_SCALE_OUTER_R   (CLOCK_FACE_CLASSIC_SEC_PIVOT_Y + 5)
#define CLOCK_FACE_CLASSIC_SCALE_COUNT     (60)
#define CLOCK_FACE_CLASSIC_SCALE_HOUR_W    (3)
#define CLOCK_FACE_CLASSIC_SCALE_HOUR_H    (14)
#define CLOCK_FACE_CLASSIC_SCALE_MIN_W     (2)
#define CLOCK_FACE_CLASSIC_SCALE_MIN_H     (7)

LV_IMG_DECLARE(clock_hand_hour);
LV_IMG_DECLARE(clock_hand_min);
LV_IMG_DECLARE(clock_hand_sec);
LV_FONT_DECLARE(ui_font_Number);

static lv_obj_t *clock_face_classic_time_label;
static lv_obj_t *clock_face_classic_date_label;
static lv_obj_t *clock_face_classic_hour_hand;
static lv_obj_t *clock_face_classic_min_hand;
static lv_obj_t *clock_face_classic_sec_hand;

static void clock_face_classic_place_hand(lv_obj_t *hand, int32_t pivot_x, int32_t pivot_y)
{
    lv_obj_set_pos(hand, CLOCK_FACE_CLASSIC_CENTER_X - pivot_x, CLOCK_FACE_CLASSIC_CENTER_Y - pivot_y);
}

static void clock_face_classic_create_scale(lv_obj_t *screen, uint32_t index)
{
    lv_obj_t *scale = NULL;
    int32_t scale_w = 0;
    int32_t scale_h = 0;
    lv_color_t scale_color = lv_color_hex(0xCDD2E8);

    if ((index % 5) == 0) {
        scale_w = CLOCK_FACE_CLASSIC_SCALE_HOUR_W;
        scale_h = CLOCK_FACE_CLASSIC_SCALE_HOUR_H;
        scale_color = lv_color_hex(0x5C679C);
    } else {
        scale_w = CLOCK_FACE_CLASSIC_SCALE_MIN_W;
        scale_h = CLOCK_FACE_CLASSIC_SCALE_MIN_H;
    }

    scale = lv_obj_create(screen);
    lv_obj_set_size(scale, scale_w, scale_h);
    lv_obj_set_pos(scale, CLOCK_FACE_CLASSIC_CENTER_X - (scale_w / 2), CLOCK_FACE_CLASSIC_CENTER_Y - CLOCK_FACE_CLASSIC_SCALE_OUTER_R);
    lv_obj_remove_flag(scale, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(scale, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(scale, scale_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scale, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(scale, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_x(scale, scale_w / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(scale, CLOCK_FACE_CLASSIC_SCALE_OUTER_R, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_rotation(scale, (int32_t)index * 60, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void clock_face_classic_create(lv_obj_t *screen)
{
    uint32_t scale_index = 0;

    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(screen, NULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_tiled(screen, false, LV_PART_MAIN | LV_STATE_DEFAULT);

    while (scale_index < CLOCK_FACE_CLASSIC_SCALE_COUNT) {
        clock_face_classic_create_scale(screen, scale_index);
        scale_index++;
    }

    clock_face_classic_min_hand = lv_image_create(screen);
    lv_image_set_src(clock_face_classic_min_hand, &clock_hand_min);
    lv_obj_add_flag(clock_face_classic_min_hand, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_remove_flag(clock_face_classic_min_hand, LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_pivot(clock_face_classic_min_hand, CLOCK_FACE_CLASSIC_MIN_PIVOT_X, CLOCK_FACE_CLASSIC_MIN_PIVOT_Y);
    clock_face_classic_place_hand(clock_face_classic_min_hand, CLOCK_FACE_CLASSIC_MIN_PIVOT_X, CLOCK_FACE_CLASSIC_MIN_PIVOT_Y);

    clock_face_classic_hour_hand = lv_image_create(screen);
    lv_image_set_src(clock_face_classic_hour_hand, &clock_hand_hour);
    lv_obj_add_flag(clock_face_classic_hour_hand, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_remove_flag(clock_face_classic_hour_hand, LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_pivot(clock_face_classic_hour_hand, CLOCK_FACE_CLASSIC_HOUR_PIVOT_X, CLOCK_FACE_CLASSIC_HOUR_PIVOT_Y);
    clock_face_classic_place_hand(clock_face_classic_hour_hand, CLOCK_FACE_CLASSIC_HOUR_PIVOT_X, CLOCK_FACE_CLASSIC_HOUR_PIVOT_Y);

    clock_face_classic_sec_hand = lv_image_create(screen);
    lv_image_set_src(clock_face_classic_sec_hand, &clock_hand_sec);
    lv_obj_add_flag(clock_face_classic_sec_hand, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_remove_flag(clock_face_classic_sec_hand, LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_pivot(clock_face_classic_sec_hand, CLOCK_FACE_CLASSIC_SEC_PIVOT_X, CLOCK_FACE_CLASSIC_SEC_PIVOT_Y);
    clock_face_classic_place_hand(clock_face_classic_sec_hand, CLOCK_FACE_CLASSIC_SEC_PIVOT_X, CLOCK_FACE_CLASSIC_SEC_PIVOT_Y);

    clock_face_classic_time_label = lv_label_create(screen);
    lv_obj_set_align(clock_face_classic_time_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(clock_face_classic_time_label, 15);
    lv_obj_set_style_text_color(clock_face_classic_time_label, lv_color_hex(0x293062), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(clock_face_classic_time_label, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(clock_face_classic_time_label, &ui_font_Number, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(clock_face_classic_time_label, LV_OBJ_FLAG_HIDDEN);

    clock_face_classic_date_label = lv_label_create(screen);
    lv_obj_set_align(clock_face_classic_date_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(clock_face_classic_date_label, 76);
    lv_obj_set_style_text_color(clock_face_classic_date_label, lv_color_hex(0x9C9CD9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(clock_face_classic_date_label, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(clock_face_classic_date_label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(clock_face_classic_date_label, LV_OBJ_FLAG_HIDDEN);
}

static void clock_face_classic_update(const clock_view_data_t *view_data)
{
    lv_label_set_text(clock_face_classic_time_label, view_data->time_text);
    lv_label_set_text(clock_face_classic_date_label, view_data->date_text);

    if (!view_data->rtc_valid) {
        return;
    }

    lv_image_set_rotation(clock_face_classic_hour_hand, view_data->hour_angle);
    lv_image_set_rotation(clock_face_classic_min_hand, view_data->min_angle);
    lv_image_set_rotation(clock_face_classic_sec_hand, view_data->sec_angle);
}

const clock_face_ops_t clock_face_classic_ops = {
    .name = "classic",
    .create = clock_face_classic_create,
    .update = clock_face_classic_update,
};
