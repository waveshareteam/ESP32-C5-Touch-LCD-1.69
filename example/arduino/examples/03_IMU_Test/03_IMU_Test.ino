#include <Arduino.h>
#include <Wire.h>
#include <esp32_c5_touch_lcd_1_69.h>
#include <FastIMU.h>

#define QMI8658_I2C_ADDRESS 0x6B

static QMI8658 imu(Wire);
static calData imu_calibration = { 0 };
static AccelData accel_data = { 0 };
static GyroData gyro_data = { 0 };

void setup(void)
{
    int init_result = -1;

    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    bsp_display_brightness_init();
    bsp_display_brightness_set(100);

    if (!bsp_i2c_init()) {
        Serial.println("I2C init failed");
        while (true) {
            delay(1000);
        }
    }

    init_result = imu.init(imu_calibration, QMI8658_I2C_ADDRESS);
    if (init_result != 0) {
        Serial.print("QMI8658 init failed: ");
        Serial.println(init_result);
        while (true) {
            delay(1000);
        }
    }

    imu.setAccelRange(8);
    imu.setGyroRange(512);
    imu.setAccelODR(1000);
    imu.setGyroODR(1000);

    Serial.println("QMI8658 ready");
}

void loop(void)
{
    imu.update();
    imu.getAccel(&accel_data);
    imu.getGyro(&gyro_data);

    Serial.print("ACC[g] ");
    Serial.print(accel_data.accelX, 3);
    Serial.print(", ");
    Serial.print(accel_data.accelY, 3);
    Serial.print(", ");
    Serial.print(accel_data.accelZ, 3);
    Serial.print("  GYRO[dps] ");
    Serial.print(gyro_data.gyroX, 3);
    Serial.print(", ");
    Serial.print(gyro_data.gyroY, 3);
    Serial.print(", ");
    Serial.print(gyro_data.gyroZ, 3);
    Serial.print("  TEMP[C] ");
    Serial.println(imu.getTemp(), 2);

    delay(100);
}
