#include "lightmeter.h"

#define WORKER_TAG "Main app"

static bool lightmeter_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    LightMeterApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool lightmeter_back_event_callback(void* context) {
    furi_assert(context);
    LightMeterApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void lightmeter_tick_event_callback(void* context) {
    furi_assert(context);
    LightMeterApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

LightMeterApp* lightmeter_app_alloc(uint32_t first_scene) {
    LightMeterApp* app = malloc(sizeof(LightMeterApp));

    // Sensor
    bh1750_set_power_state(1);
    bh1750_init();
    bh1750_set_mode(ONETIME_HIGH_RES_MODE);
    bh1750_set_mt_reg(100);

    app->config = malloc(sizeof(LightMeterConfig));
    app->config->iso = 0;
    app->config->nd = 0;
    app->config->aperture = 0;

    // Records
    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(
        app->notifications, &sequence_display_backlight_enforce_on); // force on backlight

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&lightmeter_scene_handlers, app);
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, lightmeter_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, lightmeter_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, lightmeter_tick_event_callback, furi_ms_to_ticks(200));
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Views
    app->main_view = main_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        LightMeterAppViewMainView,
        main_view_get_view(app->main_view));

    // Set default values
    main_view_set_iso(app->main_view, ISO_100);
    main_view_set_nd(app->main_view, ND_0);
    main_view_set_aperture(app->main_view, AP_2_8);
    main_view_set_speed(app->main_view, TIME_125);

    // Variable item list
    app->var_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        LightMeterAppViewVarItemList,
        variable_item_list_get_view(app->var_item_list));

    // Widget
    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, LightMeterAppViewAbout, widget_get_view(app->widget));
    view_dispatcher_add_view(
        app->view_dispatcher, LightMeterAppViewHelp, widget_get_view(app->widget));

    // Set first scene
    scene_manager_next_scene(app->scene_manager, first_scene); //! this to switch
    return app;
}

void lightmeter_app_free(LightMeterApp* app) {
    furi_assert(app);

    // Views
    view_dispatcher_remove_view(app->view_dispatcher, LightMeterAppViewMainView);
    main_view_free(app->main_view);

    // Variable item list
    view_dispatcher_remove_view(app->view_dispatcher, LightMeterAppViewVarItemList);
    variable_item_list_free(app->var_item_list);

    //  Widget
    view_dispatcher_remove_view(app->view_dispatcher, LightMeterAppViewAbout);
    view_dispatcher_remove_view(app->view_dispatcher, LightMeterAppViewHelp);
    widget_free(app->widget);

    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    // Records
    furi_record_close(RECORD_GUI);
    notification_message(
        app->notifications,
        &sequence_display_backlight_enforce_auto); // set backlight back to auto
    furi_record_close(RECORD_NOTIFICATION);

    bh1750_set_power_state(0);

    free(app);
}

int32_t lightmeter_app(void* p) {
    UNUSED(p);
    uint32_t first_scene = LightMeterAppSceneMain;
    LightMeterApp* app = lightmeter_app_alloc(first_scene);
    view_dispatcher_run(app->view_dispatcher);
    lightmeter_app_free(app);
    return 0;
}

void lightmeter_app_set_config(LightMeterApp* app, LightMeterConfig* config) {
    app->config = config;
}
