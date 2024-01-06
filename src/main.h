#pragma once

#define TAG "LightUp"

#include <furi.h>
#include <furi_hal_gpio.h>

typedef enum {
    SingleLED,
    WS8211,
} LedType;

typedef enum { LightUpScenes_Starting, LightUpScenes_GPIOTest, LightUpScenes_count } LightUpScenes;

typedef enum {
    LightUpViews_MenuView,
    LightUpViews_VariableListView,
    LightUpViews_count
} LightUpViews;

typedef struct {
    bool gpioTestPinStatus;
    // Required because there isn't a trivial way to map
    // GpioPin objects to an indexable value.
    int gpioPinIndex;
    const GpioPin* gpioPin;
    LedType ledType;
} LightUpData_t;
