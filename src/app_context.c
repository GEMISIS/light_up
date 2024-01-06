#include <gui/modules/button_menu.h>
#include <gui/modules/button_panel.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/empty_screen.h>
#include <gui/modules/file_browser.h>
#include <gui/modules/loading.h>
#include <gui/modules/menu.h>
#include <gui/modules/popup.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_box.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>

#include "main.h"
#include "app_context.h"

// Custom event handler - passes the event to the scene manager
bool viewDispatcherCustomCallback(void* context, uint32_t custom_event) {
    furi_assert(context);
    AppContext_t* appContext = context;
    return scene_manager_handle_custom_event(appContext->scene_manager, custom_event);
}

// Navigation event handler - passes the event to the scene manager
bool viewDispatcherNavigationCallback(void* context) {
    furi_assert(context);
    AppContext_t* appContext = context;
    return scene_manager_handle_back_event(appContext->scene_manager);
}

AppContextStatus initializeAppContext(
    AppContext_t** context,
    int viewsCount,
    const SceneManagerHandlers* sceneManagerHandlers) {
    FURI_LOG_D(TAG, "Allocating memory for app context");

    *context = malloc(sizeof(AppContext_t));
    if(*context == NULL) {
        FURI_LOG_E(TAG, "Failed to allocate memory for app context");
        return APP_CONTEXT_CANT_ALLOCATE;
    }

    // Allocate our scene manager with the handlers provided
    FURI_LOG_D(TAG, "Setting up the scene manager");
    (*context)->scene_manager = scene_manager_alloc(sceneManagerHandlers, *context);

    // Now setup our view dispatchers
    FURI_LOG_D(TAG, "Setting up the view dispatcher");
    (*context)->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue((*context)->view_dispatcher);

    FURI_LOG_D(TAG, "Setting view dispatcher callbacks");
    view_dispatcher_set_event_callback_context((*context)->view_dispatcher, (*context));
    FURI_LOG_D(TAG, "Setting view dispatcher custom event callbacks");
    view_dispatcher_set_custom_event_callback(
        (*context)->view_dispatcher, viewDispatcherCustomCallback);
    FURI_LOG_D(TAG, "Setting view dispatcher navigation event callbacks");
    view_dispatcher_set_navigation_event_callback(
        (*context)->view_dispatcher, viewDispatcherNavigationCallback);
    FURI_LOG_D(TAG, "Setting view dispatcher callbacks done");

    (*context)->activeViews = malloc(sizeof(View_t) * viewsCount);
    if((*context)->activeViews == NULL) {
        FURI_LOG_E(TAG, "Failed to allocate memory for active views");
        return APP_CONTEXT_CANT_ALLOCATE;
    }
    (*context)->activeViewsCount = viewsCount;

    return APP_CONTEXT_OK;
}

/// @brief Hacky structure to get the inner view nicely
typedef struct {
    View* view;
} __FlipperView__;

AppContextStatus addViewToAppContext(AppContext_t** context, View_t* view) {
    if(view->viewId > (*context)->activeViewsCount || view->viewId < 0) {
        FURI_LOG_E(TAG, "Not enough views!");
        return APP_CONTEXT_NOT_ENOUGH_VIEWS;
    }
    (*context)->activeViews[view->viewId] = view;
    View* rawView = NULL;
    switch(view->type) {
    case VIEW:
        // For normal views, simply pull the view data directly
        rawView = view->viewData;
        break;
    default:
        // For any other view types, we can pull the inner view
        // instead since it's a simple fetch.
        rawView = ((__FlipperView__*)view->viewData)->view;
        break;
    }
    if(rawView) {
        view_dispatcher_add_view((*context)->view_dispatcher, view->viewId, rawView);
        return APP_CONTEXT_OK;
    } else {
        return APP_CONTEXT_UNSUPPORTED_VIEW_TYPE;
    }
}

AppContextStatus freeAppContextViews(AppContext_t** context) {
    FURI_LOG_D(TAG, "Freeing views");
    for(int i = 0; i < (*context)->activeViewsCount; i++) {
        View_t* view = (*context)->activeViews[i];
        if(view != NULL) {
            view_dispatcher_remove_view((*context)->view_dispatcher, view->viewId);

            // Certain modules have different freeing functions, so we need
            // to actually properly free each one.
            switch(view->type) {
            case BUTTON_MENU:
                button_menu_free(view->viewData);
                break;
            case BUTTON_PANEL:
                button_panel_free(view->viewData);
                break;
            case BYTE_INPUT:
                byte_input_free(view->viewData);
                break;
            case DIALOG:
                dialog_ex_free(view->viewData);
                break;
            case EMPTY_SCREEN:
                empty_screen_free(view->viewData);
                break;
            case FILE_BROWSER:
                file_browser_free(view->viewData);
                break;
            case LOADING:
                loading_free(view->viewData);
                break;
            case MENU:
                menu_free(view->viewData);
                break;
            case POPUP:
                popup_free(view->viewData);
                break;
            case SUBMENU:
                submenu_free(view->viewData);
                break;
            case TEXT_BOX:
                text_box_free(view->viewData);
                break;
            case TEXT_INPUT:
                text_input_free(view->viewData);
                break;
            case VARIABLE_ITEM_LIST:
                variable_item_list_free(view->viewData);
                break;
            case VIEW:
                view_free(view->viewData);
                break;
            case WIDGET:
                widget_free(view->viewData);
                break;
            }
            free(view);
        }
    }
    FURI_LOG_D(TAG, "Removing all views from list");
    free((*context)->activeViews);
    (*context)->activeViewsCount = 0;
    return APP_CONTEXT_OK;
}

AppContextStatus freeAppContext(AppContext_t** context) {
    FURI_LOG_D(TAG, "Freeing views");
    AppContextStatus result = freeAppContextViews(context);
    if(result != APP_CONTEXT_OK) {
        return APP_CONTEXT_CANT_FREE_VIEWS;
    }
    FURI_LOG_D(TAG, "Freeing the scene");
    scene_manager_free((*context)->scene_manager);
    FURI_LOG_D(TAG, "Freeing the view dispatcher");
    view_dispatcher_free((*context)->view_dispatcher);
    FURI_LOG_D(TAG, "Freeing the app context");
    free((*context));
    return APP_CONTEXT_OK;
}
