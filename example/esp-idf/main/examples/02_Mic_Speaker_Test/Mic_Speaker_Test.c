#include "Mic_Speaker_Test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

#define MIC_SPEAKER_TEST_FRAME_BYTES    (1024)
#define MIC_SPEAKER_TEST_VOLUME         (70)
#define MIC_SPEAKER_TEST_GAIN           (18)

static const char *TAG = "mic_speaker_test";

void mic_speaker_test_run(void)
{
    uint8_t audio_buffer[MIC_SPEAKER_TEST_FRAME_BYTES];
    esp_codec_dev_handle_t speaker = NULL;
    esp_codec_dev_handle_t microphone = NULL;
    esp_codec_dev_sample_info_t codec_fs = {
        .sample_rate = BSP_AUDIO_OUTPUT_SAMPLE_RATE_HZ,
        .bits_per_sample = 16,
        .channel = 1,
    };

    ESP_ERROR_CHECK(bsp_audio_init(NULL));

    speaker = bsp_audio_codec_speaker_init();
    microphone = bsp_audio_codec_microphone_init();

    ESP_ERROR_CHECK(speaker ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(microphone ? ESP_OK : ESP_FAIL);

    ESP_ERROR_CHECK(esp_codec_dev_open(speaker, &codec_fs));
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(speaker, MIC_SPEAKER_TEST_VOLUME));
    ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(microphone, MIC_SPEAKER_TEST_GAIN));

    ESP_LOGI(TAG, "microphone loopback to speaker");

    while (true) {
        ESP_ERROR_CHECK(esp_codec_dev_read(microphone, audio_buffer, sizeof(audio_buffer)));
        ESP_ERROR_CHECK(esp_codec_dev_write(speaker, audio_buffer, sizeof(audio_buffer)));
    }
}
