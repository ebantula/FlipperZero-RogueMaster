#include "../shapshup_i.h"
#include "../shapshup_custom_event.h"
#include "../views/shapshup_main_view.h"
#include "../shapshup_files.h"

#define TAG "ShapShupSceneStart"

void shapshup_scene_start_callback(shapshupCustomEvent event, void* context) {
    furi_assert(context);

    shapshupState* instance = (shapshupState*)context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, event);
}

void shapshup_scene_start_on_enter(void* context) {
    furi_assert(context);

    shapshupState* instance = (shapshupState*)context;
    shapshupMainView* view = instance->view_main;
    instance->current_view = shapshupViewMain;
    shapshup_main_view_set_callback(view, shapshup_scene_start_callback, instance);
    shapshup_main_view_set_index(view, 0);
    view_dispatcher_switch_to_view(instance->view_dispatcher, instance->current_view);
}

void shapshup_scene_start_on_exit(void* context) {
    UNUSED(context);
}

bool shapshup_scene_start_on_event(void* context, SceneManagerEvent event) {
    shapshupState* instance = (shapshupState*)context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == shapshupCustomEventTypeMenuSelected) {
#ifdef FURI_DEBUG
            FURI_LOG_D(TAG, "shapshupCustomEventTypeMenuSelected");
#endif
            FuriString* value;
            value = furi_string_alloc_printf("%ld", rand_range(100, 600));
            shapshup_main_view_add_item(instance->view_main, furi_string_get_cstr(value), 0);
            furi_string_free(value);

            FlipperFormat* flipper_format = flipper_format_string_alloc();
            Stream* stream = flipper_format_get_raw_stream(flipper_format);
            if(stream_load_from_file(
                   stream, instance->storage, EXT_PATH("subghz/temp/split.sub")) <= 0) {
                FURI_LOG_E(TAG, "Error loading file");
            }

            write_file_split_shapshup(
                instance->storage, flipper_format, EXT_PATH("subghz/temp/split_proc.sub"));

            flipper_format_free(flipper_format);

            consumed = true;
        } else if(event.event == shapshupCustomEventTypeLoadFile) {
#ifdef FURI_DEBUG
            FURI_LOG_D(TAG, "shapshupCustomEventTypeLoadFile");
#endif
            scene_manager_next_scene(instance->scene_manager, ShapshupSceneLoadFile);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        //exit app
        scene_manager_stop(instance->scene_manager);
        view_dispatcher_stop(instance->view_dispatcher);
        consumed = true;
    }

    return consumed;
}