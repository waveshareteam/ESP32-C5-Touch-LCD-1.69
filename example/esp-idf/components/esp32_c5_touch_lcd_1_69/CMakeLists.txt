file(GLOB_RECURSE SRCS *.c)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES esp_driver_i2c  esp_driver_gpio esp_lcd 
    PRIV_REQUIRES esp_timer spiffs esp_psram fatfs
)

idf_component_get_property(ESP_LVGL_ADAPTER_LIB espressif__esp_lvgl_adapter COMPONENT_LIB)
idf_build_get_property(build_components BUILD_COMPONENTS)

set(lvgl_name "")
if("lvgl" IN_LIST build_components)
    set(lvgl_name "lvgl")
endif()

if((NOT lvgl_name) AND ("lvgl__lvgl" IN_LIST build_components))
    set(lvgl_name "lvgl__lvgl")
endif()

set(lvgl_ver "")
if(lvgl_name)
    idf_component_get_property(lvgl_ver ${lvgl_name} COMPONENT_VERSION)
endif()

if(ESP_LVGL_ADAPTER_LIB AND lvgl_ver VERSION_GREATER_EQUAL "9.0.0")
    target_compile_options(
        ${ESP_LVGL_ADAPTER_LIB}
        PRIVATE
            "SHELL:-Dlv_event_get_invalidated_area(e)=((lv_area_t*)lv_event_get_param(e))"
    )
endif()
