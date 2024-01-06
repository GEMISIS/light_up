#include "gpio_helper.h"
#include "../main.h"

void setGpioPin(const GpioPin* gpioPin, bool state) {
    FURI_LOG_I(TAG, "Updating pin state to %s", state ? "On" : "Off");
    if(state) {
        furi_hal_gpio_init_simple(gpioPin, GpioModeOutputPushPull);
    } else {
        furi_hal_gpio_init(gpioPin, GpioModeOutputOpenDrain, GpioPullNo, GpioSpeedLow);
    }
    furi_hal_gpio_write(gpioPin, state);
}