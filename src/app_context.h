#pragma once

#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>

/// @brief Possible types of views supported currently.
/// See https://brodan.biz/blog/a-visual-guide-to-flipper-zero-gui-components/
/// for examples of each type.
typedef enum {
    BUTTON_MENU,
    BUTTON_PANEL,
    BYTE_INPUT,
    DIALOG,
    EMPTY_SCREEN,
    FILE_BROWSER,
    LOADING,
    MENU,
    POPUP,
    SUBMENU,
    TEXT_BOX,
    TEXT_INPUT,
    VARIABLE_ITEM_LIST,
    VIEW,
    WIDGET,
} ViewType;

/// @brief Contains everything needed for managing views.
typedef struct {
    ViewType type;
    int viewId;
    void* viewData;
} View_t;

/// @brief An enum to define different result statuses for functions
/// regarding the application context.
typedef enum {
    APP_CONTEXT_OK = 0,
    APP_CONTEXT_CANT_ALLOCATE = -1,
    APP_CONTEXT_CANT_FREE_VIEWS = -2,
    APP_CONTEXT_NOT_ENOUGH_VIEWS = -3,
    APP_CONTEXT_UNSUPPORTED_VIEW_TYPE = -4,
    APP_CONTEXT_UNKNOWN_ERROR = -5,
} AppContextStatus;

/// @brief Contains everything needed for the app. Custom data
/// can be attached to the "additionalData" object.
typedef struct {
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;
    View_t** activeViews;
    int activeViewsCount;
    void* additionalData;
} AppContext_t;

/// @brief Creates an app context with the desired scene handlers.
/// @param context The app context to populate. Will be passed through to the supplied scene handlers.
/// @param viewsCount The number of views that to be added to this scene.
/// @param sceneManagerHandlers The scene handlers to use for each scene in your app.
/// @return Returns APP_CONTEXT_OK on success, APP_CONTEXT_CANT_ALLOCATE if there is an error.
AppContextStatus initializeAppContext(
    AppContext_t** context,
    int viewsCount,
    const SceneManagerHandlers* sceneManagerHandlers);

/// @brief Adds a view to the given app context.
/// @param context The app context to add the view to.
/// @param view The view to add to the app context.
/// @return Returns APP_CONTEXT_OK on success, APP_CONTEXT_NOT_ENOUGH_VIEWS if the ID of
//  the view exceeds the number of available views in the app context.
AppContextStatus addViewToAppContext(AppContext_t** context, View_t* view);

/// @brief Frees the app context entirely, cleaning it up from usage.
/// @param context The app context to clean up.
/// @return Returns APP_CONTEXT_OK on success. Should not error.
AppContextStatus freeAppContext(AppContext_t** context);
