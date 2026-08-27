#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_WS 21
#define I2S_SD 18
#define I2S_SCK 19

#define I2S_PORT I2S_NUM_0

#define bufferLen 64
int32_t sBuffer[bufferLen];

void i2s_install() {
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = bufferLen,
        .use_apll = false,
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD,
    };

    i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
    Serial.begin(115200);
    Serial.println(" ");
    delay(1000);

    i2s_install();
    i2s_setpin();
    i2s_start(I2S_PORT);

    delay(500);
}

void loop() {
    size_t bytesIn = 0;

    esp_err_t result = i2s_read(
        I2S_PORT,
        sBuffer,
        sizeof(sBuffer),
        &bytesIn,
        portMAX_DELAY
    );

    if (result == ESP_OK) {
        int slots_read = bytesIn / sizeof(int32_t);

        if (slots_read > 0) {
            for (int i = 0; i < slots_read; i += 2) {
                Serial.print(">raw:");
                Serial.println(sBuffer[i]);
            }
        }
    }
}