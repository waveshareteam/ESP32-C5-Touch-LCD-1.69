#include "IMU_Test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

#define IMU_TEST_DELAY_MS    (500)

static const char *TAG = "imu_test";

void imu_test_run(void)
{
    qmi8658_data_t data = {0};

    ESP_ERROR_CHECK(bsp_i2c_init());
    ESP_ERROR_CHECK(bsp_qmi8658_init());

    while (true) {
        ESP_ERROR_CHECK(bsp_qmi8658_get_data(&data));
        ESP_LOGI(TAG, "accel: %.2f %.2f %.2f m/s2", data.accelX, data.accelY, data.accelZ);
        ESP_LOGI(TAG, "gyro: %.2f %.2f %.2f rad/s", data.gyroX, data.gyroY, data.gyroZ);
        ESP_LOGI(TAG, "temp: %.2f C", data.temperature);
        vTaskDelay(pdMS_TO_TICKS(IMU_TEST_DELAY_MS));
    }
}
