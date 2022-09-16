#pragma once

#include "scenes/dtmf_dolphin_scene.h"

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <gui/modules/variable_item_list.h>
#include <notification/notification_messages.h>
#include <input/input.h>

#include "dtmf_dolphin_event.h"
#include "dtmf_dolphin_player.h"

#include "views/dtmf_dolphin_dialer.h"
#include "views/dtmf_dolphin_bluebox.h"

#define TAG "DTMFDolphin"


enum DTMFDolphinItem {
    DTMFDolphinItemDialer,
    DTMFDolphinItemBluebox,
    DTMFDolphinItemRedbox,
    DTMFDolphinItemPlay
};

typedef struct {
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    VariableItemList* main_menu_list;
    DTMFDolphinDialer* dtmf_dolphin_dialer;
    DTMFDolphinBluebox* dtmf_dolphin_bluebox;
    DTMFDolphinPlayer player;
    Widget* dtmf_dolphin_play;

    Gui* gui;
    // ButtonPanel* dialer_button_panel;
    // ButtonPanel* bluebox_button_panel;
    // ButtonPanel* redbox_button_panel;
    NotificationApp* notification;
} DTMFDolphinApp;

typedef enum {
    DTMFDolphinViewMainMenu,
    DTMFDolphinViewDialer,
    DTMFDolphinViewBluebox,
    DTMFDolphinViewRedbox,
    DTMFDolphinViewPlay,
} DTMFDolphinView;
