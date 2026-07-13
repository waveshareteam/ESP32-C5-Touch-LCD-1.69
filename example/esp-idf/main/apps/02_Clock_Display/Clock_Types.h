#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

typedef enum {
    CLOCK_FACE_ID_CLASSIC = 0,
    CLOCK_FACE_ID_DIGITAL,
} clock_face_id_t;

typedef struct {
    bool rtc_valid;
    uint16_t hour_angle;
    uint16_t min_angle;
    uint16_t sec_angle;
    char time_text[16];
    char date_text[24];
} clock_view_data_t;

typedef struct {
    const char *name;
    void (*create)(lv_obj_t *screen);
    void (*update)(const clock_view_data_t *view_data);
} clock_face_ops_t;
