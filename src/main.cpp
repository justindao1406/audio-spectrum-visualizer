#include <Arduino.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <FastLED.h>

// FastLED
#define LED_PIN 4
#define NUM_LEDS 120

CRGB leds[NUM_LEDS];

// INMP441
#define I2S_WS 21
#define I2S_SD 18
#define I2S_SCK 19

#define I2S_PORT I2S_NUM_0
// NUM 0 = ESP32 I2S port

#define bufferLen 64
int32_t sBuffer[bufferLen];
// 32 bits of 64 slots

constexpr int FFT_sample_size = 1024; // FFT takes 1024 samples at a time (i2s reads 64 at a time)
// 1024 / 32 = 32 i2s reads before FFT runs
int sampleIndex = 0;

float vReal[FFT_sample_size]; // stores sample data
float vImag[FFT_sample_size];

float sample_rate = 44100;

ArduinoFFT<float> FFT(vReal, vImag, FFT_sample_size, sample_rate);

void i2s_install() {
	const i2s_config_t i2s_config = {
		.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), // RX since esp32 recievies audio from INMP441 SD
		.sample_rate = 44100,
		.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // 32 bits (24 bit INMP441 SD)
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
		.communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
		.intr_alloc_flags = 0, // no interrupt
		.dma_buf_count = 8,
		.dma_buf_len = bufferLen,
		// 8 DMA * 64 elemets (of 32 bits) = 512
		// 512 total slots * 4 bytes (32 bits) = 2048 bytes
		// Note: ESP32 takes in 256 bytes at a time
		.use_apll = false,
		// clock precision = false
	};

	i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
	// no queue, pass i2s_read everytime
}

void i2s_setpin() {
	const i2s_pin_config_t pin_config = {
		.bck_io_num = I2S_SCK,
		.ws_io_num = I2S_WS,
		.data_out_num = -1, // esp32 not sending out data (speaker)
		.data_in_num = I2S_SD,
	};

	i2s_set_pin(I2S_PORT, &pin_config);
}

// LED mapping logic

int mapLED(int row, int col) {
	// return a position from continuous stirp order.
	// Starting from bottom right, then left, then up, then right, then up, then left... etc
	if (row % 2 == 0) { // even
		return (row * 12 + col);
	}
	else {
		return (row * 12 + (11 - col));
	}
}

// FFT logic

constexpr int ROWS = 10;
constexpr int COLUMNS = 12;

void runFFT() {

	FFT.dcRemoval(); // Remove k = 0
	FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); // Reduce spectral leakage
	FFT.compute(FFTDirection::Forward);
	FFT.complexToMagnitude();

	// Calculating FFT bins
	// bin width = Fs / # of samples -> 44100 / 1024 = 43.06
	// 43.06 is frequency range for a bin

	// Max frequency = 44100 / 2 = 22.05 kHz (using Nyquist Theorem)
	// Adults max out their hearing ability on avg to 16 kHz

	// Max freq / bin wdith = # of bins
	// 16 kHz / 44 Hz = 372 bins

	// 372 bins / 12 columns = 31 bins per column

	// frequency range per column = 31 * 43.06 = 1334.86 Hz

	constexpr int column_bin_size = 31;
	float noiseFloor = 0.5;

	// Finding peak amplitude for each LED column
	for (int col = 0; col < COLUMNS; col++) {
		int rowLevel = 0; // height

		int startBin = col * column_bin_size + 1;
		// + 1 because we ignore k = 0

		float peakMagnitude = 0; // for an interval of 31 bins

    if (col == 0) { // Setting a higher noise floor for highest freq. since it picks up background noise (AC, fan, vibrations... etc)
      noiseFloor = 1.9;
    }

		for (int k = 0; k < column_bin_size; k++) {
			if (vReal[startBin + k] > peakMagnitude) {
				peakMagnitude = vReal[startBin + k];
			}
		}
		if (peakMagnitude < noiseFloor) {
			peakMagnitude = 0;
			rowLevel = 0;
		}
		else if (0.5 <= peakMagnitude && peakMagnitude < 1.2) {
			rowLevel = 1;
		}
		else if (1.2 <= peakMagnitude && peakMagnitude < 2.3) {
			rowLevel = 2;
		}
		else if (2.3 <= peakMagnitude && peakMagnitude < 3.9) {
			rowLevel = 3;
		}
		else if (3.9 <= peakMagnitude && peakMagnitude < 6.2) {
			rowLevel = 4;
		}
		else if (6.2 <= peakMagnitude && peakMagnitude < 9.4) {
			rowLevel = 5;
		}
		else if (9.4 <= peakMagnitude && peakMagnitude < 13.6) {
			rowLevel = 6;
		}
		else if (13.6 <= peakMagnitude && peakMagnitude < 19.0) {
			rowLevel = 7;
		}
		else if (19.0 <= peakMagnitude && peakMagnitude < 26.0) {
			rowLevel = 8;
		}
		else if (26.0 <= peakMagnitude && peakMagnitude < 31.7) {
			rowLevel = 9;
		}
		else if (31.7 <= peakMagnitude) {
			rowLevel = 10;
		}

    int position = 0;

		for (int row = 0; row < rowLevel; row++) {
			position = mapLED(row, col);
      if (position <= 11) {
        leds[position] = 0xFF0000;
      }
      else if (12 <= position && position < 24) {
        leds[position] = 0xFF7F00;
      }
      else if (24 <= position && position < 36) {
        leds[position] = 0xFFFF00;
      }
      else if (36 <= position && position < 48) {
        leds[position] = 0x7FFF00;
      }
      else if (48 <= position && position < 60) {
        leds[position] = 0x00FF00;
      }
      else if (60 <= position && position < 72) {
        leds[position] = 0x00FF7F;
      }
      else if (72 <= position && position < 84) {
        leds[position] = 0x00FFFF;
      }
      else if (84 <= position && position < 96) {
        leds[position] = 0x007FFF;
      }
      else if (96 <= position && position < 108) {
        leds[position] = 0x0000FF;
      }
      else if (108 <= position) {
        leds[position] = 0x7F00FF;
      }
		}
	}
	Serial.println();
}

void setup() {
	Serial.begin(115200); // serial speed between ESP32 and computer
	Serial.println(" ");
	delay(1000);

	i2s_install();
	i2s_setpin();
	i2s_start(I2S_PORT);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

	delay(500);
}

void loop() {
	size_t bytesIn = 0;
	esp_err_t result = i2s_read(
		// i2s_read(port #, dest, size, bytes read, ticks to wait)
		I2S_PORT,
		sBuffer,
		sizeof(sBuffer), // CPU reads 4 (bytes) * 64 (slots) = 256 bytes
		&bytesIn,
		portMAX_DELAY // no timeout
	);
	if (result == ESP_OK)
	{
		int slots_read = bytesIn / sizeof(int32_t); // slots_read = 64 total

		// bytesIn = number of bytes received
		// each I²S slot is 32 bits = 4 bytes

		if (slots_read > 0) {
			for (int i = 0; i < slots_read; i += 2) {

				vReal[sampleIndex] = (sBuffer[i] >> 8) / 8388608.0f; // 32 bits -> 24 bit audio data. Then normalize it by 2^23 = 8388608
				vImag[sampleIndex] = 0;
				sampleIndex += 1;
			}
		}
		if (sampleIndex == FFT_sample_size) {
      FastLED.clear();
      runFFT();
      FastLED.show();

			sampleIndex = 0;
		}
	}
}