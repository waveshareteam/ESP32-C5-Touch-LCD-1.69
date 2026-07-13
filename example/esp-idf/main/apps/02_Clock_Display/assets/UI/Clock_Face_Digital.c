#include "Clock_Face_Digital.h"

#include "lvgl.h"

#define CLOCK_FACE_DIGITAL_TIME_OFFSET_Y   (-8)
#define CLOCK_FACE_DIGITAL_TIME_LETTER_SPACE (15)
#define CLOCK_FACE_DIGITAL_DATE_OFFSET_Y   (48)
#define CLOCK_FACE_DIGITAL_LINE_OFFSET_Y   (20)
#define CLOCK_FACE_DIGITAL_LINE_WIDTH      (72)
#define CLOCK_FACE_DIGITAL_LINE_HEIGHT     (2)

LV_IMG_DECLARE(ui_img_pattern_png);
LV_FONT_DECLARE(ui_font_Number);
LV_FONT_DECLARE(SC_Font_1);

static lv_obj_t *clock_face_digital_time_label;
static lv_obj_t *clock_face_digital_date_label;

static void clock_face_digital_create(lv_obj_t *screen)
{
    lv_obj_t *line = NULL;

    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_image_src(screen, &ui_img_pattern_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_tiled(screen, true, LV_PART_MAIN | LV_STATE_DEFAULT);

    clock_face_digital_time_label = lv_label_create(screen);
    lv_obj_set_align(clock_face_digital_time_label, LV_ALIGN_CENTER);
    lv_obj_set_y(clock_face_digital_time_label, CLOCK_FACE_DIGITAL_TIME_OFFSET_Y);
    lv_obj_set_style_text_color(clock_face_digital_time_label, lv_color_hex(0x1F285C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(clock_face_digital_time_label, &SC_Font_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(clock_face_digital_time_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(clock_face_digital_time_label, CLOCK_FACE_DIGITAL_TIME_LETTER_SPACE,
                                       LV_PART_MAIN | LV_STATE_DEFAULT);

    line = lv_obj_create(screen);
    lv_obj_set_size(line, CLOCK_FACE_DIGITAL_LINE_WIDTH, CLOCK_FACE_DIGITAL_LINE_HEIGHT);
    lv_obj_set_align(line, LV_ALIGN_CENTER);
    lv_obj_set_y(line, CLOCK_FACE_DIGITAL_LINE_OFFSET_Y);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(line, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x9FA7D8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(line, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);

    clock_face_digital_date_label = lv_label_create(screen);
    lv_obj_set_align(clock_face_digital_date_label, LV_ALIGN_CENTER);
    lv_obj_set_y(clock_face_digital_date_label, CLOCK_FACE_DIGITAL_DATE_OFFSET_Y);
    lv_obj_set_style_text_color(clock_face_digital_date_label, lv_color_hex(0x7C82B8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(clock_face_digital_date_label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(clock_face_digital_date_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(clock_face_digital_date_label, LV_OBJ_FLAG_HIDDEN);
}

static void clock_face_digital_update(const clock_view_data_t *view_data)
{
    lv_label_set_text(clock_face_digital_time_label, view_data->time_text);
    lv_label_set_text(clock_face_digital_date_label, view_data->date_text);
}

const clock_face_ops_t clock_face_digital_ops = {
    .name = "digital",
    .create = clock_face_digital_create,
    .update = clock_face_digital_update,
};
