#pragma once

#include <gui/scene_manager.h>

void scene_on_enter_gpio_test_scene(void* context);
bool scene_on_event_gpio_test_scene(void* context, SceneManagerEvent event);
void scene_on_exit_gpio_test_scene(void* context);
