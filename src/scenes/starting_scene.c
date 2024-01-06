#include <gui/canvas.h>

#include "starting_scene.h"
#include "../app_context.h"
#include "../main.h"

// Renders hello world
const char* test_string = "Hello World";
static void playback_view_draw_callback(Canvas* canvas, void* model) {
    UNUSED(model);

    canvas_draw_str(canvas, (128 / 2) - (strlen(test_string) * 2), 64 / 2, test_string);
}

/** resets the menu, gives it content, callbacks and selection enums */
void scene_on_enter_starting_scene(void* context) {
    FURI_LOG_I(TAG, "scene_on_enter_starting_scene");
    AppContext_t* app = (AppContext_t*)context;
    View_t* playbackView = app->activeViews[MyAppViews_StartingView];

    // Configure the custom view
    view_set_draw_callback(playbackView->viewData, playback_view_draw_callback);
    view_set_context(playbackView->viewData, context);

    // Set the currently active view
    FURI_LOG_I(TAG, "setting active view");
    view_dispatcher_switch_to_view(app->view_dispatcher, MyAppViews_StartingView);
}

/** main menu event handler - switches scene based on the event */
bool scene_on_event_starting_scene(void* context, SceneManagerEvent event) {
    FURI_LOG_I(TAG, "scene_on_event_starting_scene");
    UNUSED(context);
    UNUSED(event);
    return false;
}

void scene_on_exit_starting_scene(void* context) {
    FURI_LOG_I(TAG, "scene_on_exit_starting_scene");
    UNUSED(context);
}
