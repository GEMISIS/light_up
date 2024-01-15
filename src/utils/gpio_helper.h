#pragma once

#include <furi_hal_gpio.h>

#define MAX_LED_COUNT 10

void setGpioPin(const GpioPin* gpioPin, bool state);
void sendRgbToWS2812B(const GpioPin* gpioPin, uint32_t rgb);
