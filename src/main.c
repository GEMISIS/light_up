#include <gui/canvas.h>
#include <gui/modules/menu.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>

#include "app_context.h"
#include "main.h"

#include "scenes/starting_scene.h"

// TODO: Add scene enter handlers
// All scene on enter handlers - in the same order as their enum
void (*const scene_on_enter_handlers[])(void*) = {
    scene_on_enter_starting_scene,
};

// TODO: Add scene event handlers
// All scene on event handlers - in the same order as their enum
bool (*const scene_on_event_handlers[])(void*, SceneManagerEvent) = {
    scene_on_event_starting_scene,
};

// TODO: Add scene exit handlers
// All scene on exit handlers - in the same order as their enum
void (*const scene_on_exit_handlers[])(void*) = {
    scene_on_exit_starting_scene,
};

const SceneManagerHandlers scene_event_handlers = {
    .on_enter_handlers = scene_on_enter_handlers,
    .on_event_handlers = scene_on_event_handlers,
    .on_exit_handlers = scene_on_exit_handlers,
    .scene_num = MyAppScenes_count};

int setupViews(AppContext_t** appContext) {
    // TODO: Create views here for your app
    FURI_LOG_I(TAG, "Creating views");
    View_t* startingView = malloc(sizeof(View_t));
    startingView->viewData = view_alloc();
    startingView->viewId = MyAppViews_StartingView;
    startingView->type = VIEW;

    // Add views to the app context to be managed there
    FURI_LOG_I(TAG, "Adding views to app context");
    AppContextStatus result = addViewToAppContext(appContext, startingView);
    if(result != APP_CONTEXT_OK) {
        FURI_LOG_E(TAG, "There was a problem adding the view %d!", startingView->viewId);
        return -1;
    }

    return 0;
}

int32_t main_entry(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Starting the app...");

    AppContext_t* appContext;
    AppContextStatus result =
        initializeAppContext(&appContext, MyAppViews_count, &scene_event_handlers);

    if(result == APP_CONTEXT_OK) {
        result = setupViews(&appContext);
        if(result == 0) {
            // set the scene and launch the main loop
            FURI_LOG_D(TAG, "Setting the scene");
            Gui* gui = furi_record_open(RECORD_GUI);
            view_dispatcher_attach_to_gui(
                appContext->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
            // TODO: Update with name of starting scene
            scene_manager_next_scene(appContext->scene_manager, MyAppScenes_Starting);
            FURI_LOG_D(TAG, "Starting the view dispatcher");
            view_dispatcher_run(appContext->view_dispatcher);
        }

        // free all memory
        FURI_LOG_D(TAG, "Ending the app");
        furi_record_close(RECORD_GUI);
        freeAppContext(&appContext);
        return 0;
    }
    return -1;
}
