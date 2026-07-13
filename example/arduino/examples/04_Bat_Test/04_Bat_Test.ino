#include <Arduino.h>
#include <esp32_c5_touch_lcd_1_69.h>
#include <lvgl/lvgl.h>

static const uint16_t battery_capacity_mah = 1500;

static const bsp_display_lvgl_partial_cfg_t battery_display_cfg = {
    .use_psram = false,
    .double_buffer = false,
    .buffer_height = 120,
};

static lv_obj_t *battery_label = NULL;
static bool battery_label_has_sample = false;
static bsp_bat_info_t battery_last_info = {};

static bool bat_info_changed(const bsp_bat_info_t *lhs, const bsp_bat_info_t *rhs)
{
    if ((lhs == NULL) || (rhs == NULL)) {
        return true;
    }

    return (lhs->mv != rhs->mv) ||
        (lhs->ma != rhs->ma) ||
        (lhs->soc != rhs->soc) ||
        (lhs->tc != rhs->tc) ||
        (lhs->mah_rem != rhs->mah_rem) ||
        (lhs->mah_fcc != rhs->mah_fcc);
}

static void bat_update_label(const bsp_bat_info_t *bat_info)
{
    const char *battery_state = "Idle";
    const char *battery_note = "";

    if (battery_label == NULL) {
        return;
    }

    if (bat_info == NULL) {
        lv_label_set_text(battery_label, "Battery Test\nBattery status updating");
        return;
    }

    battery_state = (bat_info->ma > 0) ? "Charging" : ((bat_info->ma < 0) ? "Discharging" : "Idle");
    battery_note = (bat_info->ma == 0) ? "battery not connected or full" : "battery connected";

    lv_label_set_text_fmt(battery_label,
        "Battery Test\n"
        "State: %s\n"
        "Voltage: %u mV\n"
        "Current: %d mA\n"
        "SOC: %u %%\n"
        "Temp: %d C\n"
        "Capacity: %u mAh\n"
        "%s",
        battery_state,
        bat_info->mv,
        bat_info->ma,
        bat_info->soc,
        bat_info->tc,
        battery_capacity_mah,
        battery_note);
}

void setup(void)
{
    bsp_bat_info_t bat_info = {};
    void *display = NULL;

    Serial.begin(115200);

    bsp_display_start_partial(&battery_display_cfg);

    bsp_display_brightness_set(100);
    bsp_bat_init(battery_capacity_mah);

    bsp_display_lock(0);
    battery_label = lv_label_create(lv_scr_act());
    lv_obj_align(battery_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(battery_label, "Battery Test\nBattery status updating");
    bsp_display_unlock();

    if (bsp_get_bat_info(&bat_info)) {
        if (bsp_display_lock(0)) {
            bat_update_label(&bat_info);
            battery_last_info = bat_info;
            battery_label_has_sample = true;
            bsp_display_unlock();
        }
    }
}

void loop(void)
{
    bsp_bat_info_t bat_info = {};
    const char *battery_state = "Idle";
    const char *battery_note = "";

    if (!bsp_get_bat_info(&bat_info)) {
        Serial.println("battery info update failed");
        if (bsp_display_lock(0)) {
            lv_label_set_text(battery_label, "Battery Test\nBattery status updating");
            bsp_display_unlock();
        }
        return;
    }

    battery_state = (bat_info.ma > 0) ? "Charging" : ((bat_info.ma < 0) ? "Discharging" : "Idle");
    battery_note = (bat_info.ma == 0) ? "battery not connected or full" : "battery connected";

    Serial.print("state=");
    Serial.print(battery_state);
    Serial.print(" voltage=");
    Serial.print(bat_info.mv);
    Serial.print(" current=");
    Serial.print(bat_info.ma);
    Serial.print(" soc=");
    Serial.print(bat_info.soc);
    Serial.print(" temp=");
    Serial.print(bat_info.tc);
    Serial.print(" rem=");
    Serial.print(bat_info.mah_rem);
    Serial.print(" fcc=");
    Serial.println(bat_info.mah_fcc);

    if (!battery_label_has_sample || bat_info_changed(&battery_last_info, &bat_info)) {
        if (bsp_display_lock(0)) {
            bat_update_label(&bat_info);
            battery_last_info = bat_info;
            battery_label_has_sample = true;
            bsp_display_unlock();
        } else {
            Serial.println("display lock failed");
        }
    }

    delay(1000);
}
