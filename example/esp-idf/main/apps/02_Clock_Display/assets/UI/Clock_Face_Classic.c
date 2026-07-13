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

LV_IMG_DECLARE(Clock_Background);
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

static void clock_face_classic_create(lv_obj_t *screen)
{
    lv_obj_t *background = NULL;
    lv_obj_t *dot_bar = NULL;
    size_t index = 0;

    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(screen, NULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_tiled(screen, false, LV_PART_MAIN | LV_STATE_DEFAULT);

    background = lv_image_create(screen);
    lv_image_set_src(background, &Clock_Background);
    lv_obj_set_align(background, LV_ALIGN_CENTER);
    lv_obj_remove_flag(background, LV_OBJ_FLAG_SCROLLABLE);

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

    dot_bar = lv_obj_create(screen);
    lv_obj_set_size(dot_bar, 65, 12);
    lv_obj_set_align(dot_bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(dot_bar, -8);
    lv_obj_remove_flag(dot_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(dot_bar, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dot_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    index = 0;
    while (index < 6) {
        lv_obj_t *dot = lv_obj_create(dot_bar);

        lv_obj_set_size(dot, (index == 0) ? 8 : 4, (index == 0) ? 8 : 4);
        lv_obj_set_align(dot, LV_ALIGN_LEFT_MID);
        lv_obj_set_x(dot, (int32_t)(index * 10));
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(dot, (index == 0) ? lv_color_hex(0x101C52) : lv_color_hex(0xB4B6E6),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        index++;
    }
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
