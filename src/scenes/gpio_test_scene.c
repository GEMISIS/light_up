#include <gui/modules/variable_item_list.h>
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>

#include "gpio_test_scene.h"
#include "../utils/gpio_helper.h"
#include "../app_context.h"
#include "../main.h"

static void testLed(const LightUpData_t* lightUpData) {
    switch(lightUpData->ledType) {
    case SingleLED:
        setGpioPin(lightUpData->gpioPin, lightUpData->gpioTestPinStatus);
        break;
    default:
        FURI_LOG_E(TAG, "Error with selected LED type %d", lightUpData->ledType);
        break;
    }
}

static char* gpio_pin_option_names[] = {"A7", "A6", "A4", "B3", "B2", "C3"};
static void gpio_pin_type_option_change(VariableItem* item) {
    AppContext_t* app = variable_item_get_context(item);
    LightUpData_t* lightUpData = ((LightUpData_t*)app->additionalData);

    lightUpData->gpioPinIndex = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, gpio_pin_option_names[lightUpData->gpioPinIndex]);

    // Turn the GPIO pin off first
    setGpioPin(lightUpData->gpioPin, false);
    switch(lightUpData->gpioPinIndex) {
    case 5:
        lightUpData->gpioPin = &gpio_ext_pc3;
        break;
    case 4:
        lightUpData->gpioPin = &gpio_ext_pb2;
        break;
    case 3:
        lightUpData->gpioPin = &gpio_ext_pb3;
        break;
    case 2:
        lightUpData->gpioPin = &gpio_ext_pa4;
        break;
    case 1:
        lightUpData->gpioPin = &gpio_ext_pa6;
        break;
    default:
    case 0:
        lightUpData->gpioPin = &gpio_ext_pa7;
        break;
    }
    testLed(lightUpData);
}

static uint8_t gpio_option_values[] = {0, 1};
static char* gpio_option_names[] = {"Off", "On"};
static void gpio_type_option_change(VariableItem* item) {
    AppContext_t* app = variable_item_get_context(item);
    LightUpData_t* lightUpData = ((LightUpData_t*)app->additionalData);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, gpio_option_names[index]);
    // Turn the GPIO pin off first
    setGpioPin(lightUpData->gpioPin, false);
    lightUpData->gpioTestPinStatus = index;
    testLed(lightUpData);
}

/** resets the menu, gives it content, callbacks and selection enums */
void scene_on_enter_gpio_test_scene(void* context) {
    FURI_LOG_I(TAG, "scene_on_enter_gpio_test_scene");
    AppContext_t* app = (AppContext_t*)context;
    View_t* variableItemListView = app->activeViews[LightUpViews_VariableListView];

    variable_item_list_reset(variableItemListView->viewData);

    VariableItem* item = variable_item_list_add(
        variableItemListView->viewData,
        "Pin Status",
        COUNT_OF(gpio_option_values),
        gpio_type_option_change,
        app);
    variable_item_set_current_value_index(
        item, ((LightUpData_t*)app->additionalData)->gpioTestPinStatus);
    variable_item_set_current_value_text(
        item, gpio_option_names[((LightUpData_t*)app->additionalData)->gpioTestPinStatus]);

    item = variable_item_list_add(
        variableItemListView->viewData, "Selected Pin", 6, gpio_pin_type_option_change, app);

    variable_item_set_current_value_index(
        item, ((LightUpData_t*)app->additionalData)->gpioPinIndex);
    variable_item_set_current_value_text(
        item, gpio_pin_option_names[((LightUpData_t*)app->additionalData)->gpioPinIndex]);

    // Set the currently active view
    FURI_LOG_I(TAG, "setting active view");
    view_dispatcher_switch_to_view(app->view_dispatcher, LightUpViews_VariableListView);
}

/** main menu event handler - switches scene based on the event */
bool scene_on_event_gpio_test_scene(void* context, SceneManagerEvent event) {
    FURI_LOG_I(TAG, "scene_on_event_gpio_test_scene");
    UNUSED(context);
    UNUSED(event);
    return false;
}

void scene_on_exit_gpio_test_scene(void* context) {
    FURI_LOG_I(TAG, "scene_on_exit_gpio_test_scene");
    AppContext_t* app = (AppContext_t*)context;
    LightUpData_t* lightUpData = ((LightUpData_t*)app->additionalData);
    lightUpData->gpioTestPinStatus = false;
    setGpioPin(lightUpData->gpioPin, false);
}
