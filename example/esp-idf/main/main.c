#include "examples/01_RGB_Test/RGB_Test.h"
#include "examples/02_Mic_Speaker_Test/Mic_Speaker_Test.h"
#include "examples/03_IMU_Test/IMU_Test.h"
#include "examples/04_Bat_Test/Bat_Test.h"
#include "examples/05_RTC_Test/RTC_Test.h"
#include "examples/06_LVGL_Demo_Test/LVGL_Demo_Test.h"
#include "examples/07_Brookesia_Test/Brookesia_Test.h"
#include "apps/01_WIFI_Connect/WiFi_Connect.h"
#include "apps/02_Clock_Display/Clock_App.h"


// Only one example code can be run at a time
#define EXAMPLE_RGB_TEST            1
#define EXAMPLE_MIC_SPEAKER_TEST    0
#define EXAMPLE_IMU_TEST            0
#define EXAMPLE_BAT_TEST            0
#define EXAMPLE_RTC_TEST            0
#define EXAMPLE_LVGL_DEMO_TEST      0
#define EXAMPLE_Brookesia_TEST      0

#define APPS_WIFI_Connect           0
#define APPS_Clock                  0

void app_main(void)
{
#if EXAMPLE_RGB_TEST
    rgb_test_run();
#elif EXAMPLE_MIC_SPEAKER_TEST
    mic_speaker_test_run();
#elif EXAMPLE_IMU_TEST
    imu_test_run();
#elif EXAMPLE_BAT_TEST
    bat_test_run(1000);  //input battery capacity in mAh
#elif EXAMPLE_RTC_TEST
    rtc_test_run();
#elif EXAMPLE_LVGL_DEMO_TEST
    lvgl_test_run();
#elif EXAMPLE_Brookesia_TEST
    brookesia_test_run();
#elif APPS_WIFI_Connect
    wifi_connect_test_run();
#elif APPS_Clock
    clock_test_run();
#endif
}
