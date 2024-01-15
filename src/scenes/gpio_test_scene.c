#include <gui/modules/variable_item_list.h>
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>
#include <furi_hal_power.h>

#include "gpio_test_scene.h"
#include "../utils/gpio_helper.h"
#include "../app_context.h"
#include "../main.h"

/**
 * Colors in the format of BGR
*/
typedef enum {
    Red = 0xff0000,
    Green = 0x00ff00,
    Blue = 0x0000ff,
} LightColors;

static LightColors gpio_light_color_options[] = {Red, Green, Blue};
static void testLed(const LightUpData_t* lightUpData) {
    switch(lightUpData->ledType) {
    case SingleLED:
        setGpioPin(lightUpData->gpioPin, lightUpData->gpioTestPinStatus);
        break;
    case WS8211:
        if(lightUpData->gpioTestPinStatus) {
            // furi_hal_power_enable_otg();
            // sendRgbToWS8211(
            //     lightUpData->gpioPin, gpio_light_color_options[lightUpData->lightColorSelection]);
        }
        break;
    case WS2812B:
        if(lightUpData->gpioTestPinStatus) {
            furi_hal_power_enable_otg();
            sendRgbToWS2812B(
                lightUpData->gpioPin, gpio_light_color_options[lightUpData->lightColorSelection]);
        }
        break;
    default:
        FURI_LOG_E(TAG, "Error with selected LED type %d", lightUpData->ledType);
        break;
    }
}

static char* gpio_pin_status_names[] = {"Off", "On"};
static void gpio_pin_status_change(VariableItem* item) {
    AppContext_t* app = variable_item_get_context(item);
    LightUpData_t* lightUpData = ((LightUpData_t*)app->additionalData);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, gpio_pin_status_names[index]);
    // Turn the GPIO pin off first
    setGpioPin(lightUpData->gpioPin, false);
    // Always power cycle to clear the previous lights
    furi_hal_power_disable_otg();
    lightUpData->gpioTestPinStatus = index;
    testLed(lightUpData);
}

static char* gpio_selected_pin_names[] = {"A7", "A6", "A4", "B3", "B2", "C3"};
static const GpioPin* gpio_selected_pin_options[] =
    {&gpio_ext_pa7, &gpio_ext_pa6, &gpio_ext_pa4, &gpio_ext_pb3, &gpio_ext_pb2, &gpio_ext_pc3};
static void gpio_selected_pin_change(VariableItem* item) {
    AppContext_t* app = variable_item_get_context(item);
    LightUpData_t* lightUpData = ((LightUpData_t*)app->additionalData);

    lightUpData->gpioPinIndex = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, gpio_selected_pin_names[lightUpData->gpioPinIndex]);

    // Turn the GPIO pin off first
    setGpioPin(lightUpData->gpioPin, false);
    lightUpData->gpioPin = gpio_selected_pin_options[lightUpData->gpioPinIndex];
    testLed(lightUpData);
}

static char* gpio_pin_led_type_names[] = {"Circuit", "WS8211", "WS2812B"};
static void gpio_pin_led_type_change(VariableItem* item) {
    AppContext_t* app = variable_item_get_context(item);
    LightUpData_t* lightUpData = ((LightUpData_t*)app->additionalData);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, gpio_pin_led_type_names[index]);
    lightUpData->ledType = index;
    // Turn the GPIO pin off first
    setGpioPin(lightUpData->gpioPin, false);
    // Always power cycle to clear the previous lights
    furi_hal_power_disable_otg();
    testLed(lightUpData);
}

static char* gpio_light_color_names[] = {"Red", "Green", "Blue"};
static void gpio_light_color_change(VariableItem* item) {
    AppContext_t* app = variable_item_get_context(item);
    LightUpData_t* lightUpData = ((LightUpData_t*)app->additionalData);
    lightUpData->lightColorSelection = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(
        item, gpio_light_color_names[lightUpData->lightColorSelection]);
    // Turn the GPIO pin off first
    testLed(lightUpData);
}

/** resets the menu, gives it content, callbacks and selection enums */
void scene_on_enter_gpio_test_scene(void* context) {
    FURI_LOG_I(TAG, "scene_on_enter_gpio_test_scene");
    AppContext_t* app = (AppContext_t*)context;
    View_t* variableItemListView = app->activeViews[LightUpViews_VariableListView];

    // Add status options
    variable_item_list_reset(variableItemListView->viewData);

    VariableItem* item = variable_item_list_add(
        variableItemListView->viewData, "Pin Status", 2, gpio_pin_status_change, app);
    variable_item_set_current_value_index(
        item, ((LightUpData_t*)app->additionalData)->gpioTestPinStatus);
    variable_item_set_current_value_text(
        item, gpio_pin_status_names[((LightUpData_t*)app->additionalData)->gpioTestPinStatus]);

    // Add pin selection options
    item = variable_item_list_add(
        variableItemListView->viewData, "Selected Pin", 6, gpio_selected_pin_change, app);

    variable_item_set_current_value_index(
        item, ((LightUpData_t*)app->additionalData)->gpioPinIndex);
    variable_item_set_current_value_text(
        item, gpio_selected_pin_names[((LightUpData_t*)app->additionalData)->gpioPinIndex]);

    // Add led type options
    item = variable_item_list_add(
        variableItemListView->viewData, "LED Type", LedTypeSize, gpio_pin_led_type_change, app);

    variable_item_set_current_value_index(item, ((LightUpData_t*)app->additionalData)->ledType);
    variable_item_set_current_value_text(
        item, gpio_pin_led_type_names[((LightUpData_t*)app->additionalData)->ledType]);

    // Add light color for where available
    item = variable_item_list_add(
        app->activeViews[LightUpViews_VariableListView]->viewData,
        "Color",
        COUNT_OF(gpio_light_color_names),
        gpio_light_color_change,
        app);

    variable_item_set_current_value_index(
        item, ((LightUpData_t*)app->additionalData)->lightColorSelection);
    variable_item_set_current_value_text(
        item, gpio_light_color_names[((LightUpData_t*)app->additionalData)->lightColorSelection]);

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
    furi_hal_power_disable_otg();
}
