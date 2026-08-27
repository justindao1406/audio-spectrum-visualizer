#include <Arduino.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <FastLED.h>

#define I2S_WS 21
#define I2S_SD 18
#define I2S_SCK 19

#define I2S_PORT I2S_NUM_0

#define bufferLen 64
int32_t sBuffer[bufferLen];

constexpr int FFT_sample_size = 1024; 
int sampleIndex = 0;

float vReal[FFT_sample_size]; 
float vImag[FFT_sample_size];

float sample_rate = 44100;

ArduinoFFT<float> FFT(vReal, vImag, FFT_sample_size, sample_rate);

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

// FFT logic

void runFFT() {

	FFT.dcRemoval(); // Remove k = 0
	FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); 
	FFT.compute(FFTDirection::Forward);
	FFT.complexToMagnitude();

	// Calculating FFT bins
	// bin width = Fs / # of samples -> 44100 / 1024 = 43.06
	// 43.06 is frequency range for a bin

	// Max frequency = 44100 / 2 = 22.05 kHz (using Nyquist Theorem)
	// Adults max out their hearing ability on avg to 16 kHz

	// Max freq / bin width = # of bins
	// 16 kHz / 44 Hz = 372 bins

    float peakMagnitude = 0;
    int peakBin = 0;
    float peakFrequency = 0;

    for (int k = 1; k <= 372; k++) {
        if (peakMagnitude < vReal[k]) {
            peakMagnitude = vReal[k];
            peakBin = k;
            peakFrequency = peakBin * (sample_rate / FFT_sample_size);
        }
    }

    Serial.print("Peak bin: ");
    Serial.println(peakBin);
    Serial.print("Peak magnitude: ");
    Serial.println(peakMagnitude);
    Serial.print("Peak Frequency: ");
    Serial.println(peakFrequency);
	Serial.println();
}

void setup() {
	Serial.begin(115200); // serial speed between ESP32 and computer
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
	if (result == ESP_OK)
	{
		int slots_read = bytesIn / sizeof(int32_t); 
		if (slots_read > 0) {
			for (int i = 0; i < slots_read; i += 2) {

				vReal[sampleIndex] = (sBuffer[i] >> 8) / 8388608.0f; 
				vImag[sampleIndex] = 0;
				sampleIndex += 1;
			}
		}
		if (sampleIndex == FFT_sample_size) {
            runFFT();
			sampleIndex = 0;
		}
	}
}