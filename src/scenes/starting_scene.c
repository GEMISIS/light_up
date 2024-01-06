#include <gui/modules/menu.h>

#include "starting_scene.h"
#include "../app_context.h"
#include "../main.h"

typedef enum {
    LightUpAppMenuSelection_RunLights,
    LightUpAppMenuSelection_TestGPIO,
} LightUpAppMenuSelection;

void menu_callback_starting_scene(void* context, uint32_t index) {
    FURI_LOG_I(TAG, "menu_callback_starting_scene");
    scene_manager_handle_custom_event(((AppContext_t*)context)->scene_manager, index);
}

/** resets the menu, gives it content, callbacks and selection enums */
void scene_on_enter_starting_scene(void* context) {
    FURI_LOG_I(TAG, "scene_on_enter_starting_scene");
    AppContext_t* app = (AppContext_t*)context;
    View_t* menuView = app->activeViews[LightUpViews_MenuView];

    // Set the currently active view
    menu_reset(menuView->viewData);

    menu_add_item(
        menuView->viewData,
        "Run Lights",
        NULL,
        LightUpAppMenuSelection_RunLights,
        menu_callback_starting_scene,
        app);
    menu_add_item(
        menuView->viewData,
        "GPIO Tester",
        NULL,
        LightUpAppMenuSelection_TestGPIO,
        menu_callback_starting_scene,
        app);

    // Set the currently active view
    FURI_LOG_I(TAG, "setting active view");
    view_dispatcher_switch_to_view(app->view_dispatcher, LightUpViews_MenuView);
}

bool scene_on_event_starting_scene(void* context, SceneManagerEvent event) {
    FURI_LOG_I(TAG, "scene_on_event_starting_scene");
    AppContext_t* app = context;
    bool consumed = false;
    switch(event.type) {
    case SceneManagerEventTypeCustom:
        switch(event.event) {
        case LightUpAppMenuSelection_RunLights:
            // scene_manager_next_scene(app->scene_manager, LightUpScenes_RunLights);
            consumed = true;
            break;
        case LightUpAppMenuSelection_TestGPIO:
            scene_manager_next_scene(app->scene_manager, LightUpScenes_GPIOTest);
            consumed = true;
            break;
        }
        break;
    default: // eg. SceneManagerEventTypeBack, SceneManagerEventTypeTick
        consumed = false;
        break;
    }
    return consumed;
    return false;
}

void scene_on_exit_starting_scene(void* context) {
    FURI_LOG_I(TAG, "scene_on_exit_starting_scene");
    UNUSED(context);
}
