#include <esp32_c5_touch_lcd_1_69.h>

namespace {


constexpr size_t audio_frame_bytes = 1024;
constexpr uint8_t speaker_volume = 70;
constexpr uint8_t mic_gain_db = 18;

uint8_t audio_buffer[audio_frame_bytes];
bool audio_ready = false;

void wait_serial_briefly(void)
{
    unsigned long start_ms = millis();

    while (!Serial && (millis() - start_ms < 200)) {
        delay(10);
    }
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    wait_serial_briefly();

    if (!bsp_audio_init()) {
        Serial.println("bsp_audio_init failed");
        return;
    }

    if (!bsp_audio_set_speaker_volume(speaker_volume)) {
        Serial.println("bsp_audio_set_speaker_volume failed");
        return;
    }

    if (!bsp_audio_set_mic_gain(mic_gain_db)) {
        Serial.println("bsp_audio_set_mic_gain failed");
        return;
    }

    audio_ready = true;
    Serial.println("microphone loopback to speaker");
}

void loop()
{
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    if (!audio_ready) {
        delay(1000);
        return;
    }

    if (!bsp_audio_read(audio_buffer, sizeof(audio_buffer), &bytes_read) || (bytes_read == 0)) {
        Serial.println("bsp_audio_read failed");
        delay(10);
        return;
    }

    if (!bsp_audio_write(audio_buffer, bytes_read, &bytes_written) || (bytes_written != bytes_read)) {
        Serial.println("bsp_audio_write failed");
        delay(10);
    }
}
