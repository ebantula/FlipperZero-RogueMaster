#include "lightmeter.h"

static bool lightmeter_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    LightMeterApp* lightmeter = context;
    return scene_manager_handle_custom_event(lightmeter->scene_manager, event);
}

static bool lightmeter_back_event_callback(void* context) {
    furi_assert(context);
    LightMeterApp* lightmeter = context;
    return scene_manager_handle_back_event(lightmeter->scene_manager);
}

static void lightmeter_tick_event_callback(void* context) {
    furi_assert(context);
    LightMeterApp* lightmeter = context;
    scene_manager_handle_tick_event(lightmeter->scene_manager);
}

LightMeterApp* lightmeter_app_alloc(uint32_t first_scene) {
    LightMeterApp* lightmeter = malloc(sizeof(LightMeterApp));

    // Records
    lightmeter->gui = furi_record_open(RECORD_GUI);

    // View dispatcher
    lightmeter->view_dispatcher = view_dispatcher_alloc();
    lightmeter->scene_manager = scene_manager_alloc(&lightmeter_scene_handlers, lightmeter);
    view_dispatcher_enable_queue(lightmeter->view_dispatcher);
    view_dispatcher_set_event_callback_context(lightmeter->view_dispatcher, lightmeter);
    view_dispatcher_set_custom_event_callback(
        lightmeter->view_dispatcher, lightmeter_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        lightmeter->view_dispatcher, lightmeter_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        lightmeter->view_dispatcher, lightmeter_tick_event_callback, 2000);
    view_dispatcher_attach_to_gui(lightmeter->view_dispatcher, lightmeter->gui, ViewDispatcherTypeFullscreen);

    // Views
    lightmeter->main_view = main_view_alloc();
    view_dispatcher_add_view(
        lightmeter->view_dispatcher, VirtualButtonAppViewMainView, main_view_get_view(lightmeter->main_view));

    // app->modul_view = module_view_alloc();
    // view_dispatcher_add_view(
    //     app->view_dispatcher,
    //     VirtualButtonAppViewModuleView,
    //     module_view_get_view(app->modul_view));

    // app->reboo_view = reboot_view_alloc();
    // view_dispatcher_add_view(
    //     app->view_dispatcher,
    //     VirtualButtonAppViewRebootView,
    //     reboot_view_get_view(app->reboo_view));

    // app->rese_view = reset_view_alloc();
    // view_dispatcher_add_view(
    //     app->view_dispatcher, VirtualButtonAppViewResetView, reset_view_get_view(app->rese_view));

    // app->submenu = submenu_alloc();
    // view_dispatcher_add_view(
    //     app->view_dispatcher, VirtualButtonAppViewSubmenu, submenu_get_view(app->submenu));
    // app->dialog = dialog_ex_alloc();
    // view_dispatcher_add_view(
    //     app->view_dispatcher, VirtualButtonAppViewDialog, dialog_ex_get_view(app->dialog));

    // Set first scene
    scene_manager_next_scene(lightmeter->scene_manager, first_scene); //! this to switch
    return lightmeter;
}

void lightmeter_app_free(LightMeterApp* lightmeter) {
    furi_assert(lightmeter);
    // Views
    view_dispatcher_remove_view(lightmeter->view_dispatcher, VirtualButtonAppViewMainView);
    main_view_free(lightmeter->main_view);
    // view_dispatcher_remove_view(app->view_dispatcher, VirtualButtonAppViewModuleView);
    // module_view_free(app->modul_view);
    // view_dispatcher_remove_view(app->view_dispatcher, VirtualButtonAppViewRebootView);
    // reboot_view_free(app->reboo_view);
    // view_dispatcher_remove_view(app->view_dispatcher, VirtualButtonAppViewResetView);
    // reset_view_free(app->rese_view);
    // view_dispatcher_remove_view(app->view_dispatcher, VirtualButtonAppViewSubmenu);
    // submenu_free(app->submenu);
    // view_dispatcher_remove_view(app->view_dispatcher, VirtualButtonAppViewDialog);
    // dialog_ex_free(app->dialog);
    // View dispatcher
    view_dispatcher_free(lightmeter->view_dispatcher);
    scene_manager_free(lightmeter->scene_manager);
    // Records
    furi_record_close(RECORD_GUI);
    free(lightmeter);
}

int32_t lightmeter_app(void* p) {
    UNUSED(p);
    uint32_t first_scene = LightMeterAppSceneMain;
    LightMeterApp* lightmeter = lightmeter_app_alloc(first_scene);
    view_dispatcher_run(lightmeter->view_dispatcher);
    lightmeter_app_free(lightmeter);
    return 0;
}
