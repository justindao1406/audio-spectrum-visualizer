#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN 4
#define NUM_LEDS 120

CRGB leds[NUM_LEDS]; 

void setup() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50); // 0 - 255
}

void loop() {
    for (int i = 0; i < NUM_LEDS; i++) { // going up

        if (i <= 11) {
        leds[i] = 0xFF0000;
        }
        else if (12 <= i && i < 24) {
        leds[i] = 0xFF4500;
        }
        else if (24 <= i && i < 36) {
        leds[i] = 0xFFFF00;
        }
        else if (36 <= i && i < 48) {
        leds[i] = 0x7FFF00;
        }
        else if (48 <= i && i < 60) {
        leds[i] = 0x00FF00;
        }
        else if (60 <= i && i < 72) {
        leds[i] = 0x00FF7F;
        }
        else if (72 <= i && i < 84) {
        leds[i] = 0x007FFF;
        }
        else if (84 <= i && i < 96) {
        leds[i] = 0x0000FF;
        }
        else if (96 <= i && i < 108) {
        leds[i] = 0x7F00FF;
        }
        else if (108 <= i) {
        leds[i] = 0xFF00FF;
        }

        FastLED.show();
        delay(50);
        leds[i] = CRGB::Black;
    }

    for (int i = NUM_LEDS - 1; i >= 0; i--) {

        if (i <= 11) {
        leds[i] = 0xFF0000;
        }
        else if (12 <= i && i < 24) {
        leds[i] = 0xFF4500;
        }
        else if (24 <= i && i < 36) {
        leds[i] = 0xFFFF00;
        }
        else if (36 <= i && i < 48) {
        leds[i] = 0x7FFF00;
        }
        else if (48 <= i && i < 60) {
        leds[i] = 0x00FF00;
        }
        else if (60 <= i && i < 72) {
        leds[i] = 0x00FF7F;
        }
        else if (72 <= i && i < 84) {
        leds[i] = 0x007FFF;
        }
        else if (84 <= i && i < 96) {
        leds[i] = 0x0000FF;
        }
        else if (96 <= i && i < 108) {
        leds[i] = 0x7F00FF;
        }
        else if (108 <= i) {
        leds[i] = 0xFF00FF;
        }

        FastLED.show();
        delay(50);
        leds[i] = CRGB::Black;
    }
}