#include "main_view.h"
#include <furi.h>
#include <furi_hal.h>
#include <gui/elements.h>
#include "../../lightmeter.h"
#include "../../lightmeter_helper.h"
// #include <notification/notification.h>
// #include <notification/notification_messages.h>

#define WORKER_TAG "Main View"

static const int iso_numbers[] = {
    // [ISO_6] = 6,
    // [ISO_12] = 12,
    // [ISO_25] = 25,
    // [ISO_50] = 50,
    [ISO_100] = 100,
    [ISO_200] = 200,
    [ISO_400] = 400,
    [ISO_800] = 800,
    [ISO_1600] = 1600,
    [ISO_3200] = 3200,
    [ISO_6400] = 6400,
    [ISO_12800] = 12800,
    [ISO_25600] = 25600,
    [ISO_51200] = 51200,
    [ISO_102400] = 102400,
};

static const int nd_numbers[] = {
    [ND_0] = 0,
    [ND_2] = 2,
    [ND_4] = 4,
    [ND_8] = 8,
    [ND_16] = 16,
    [ND_32] = 32,
    [ND_64] = 64,
    [ND_128] = 128,
    [ND_256] = 256,
    [ND_512] = 512,
    [ND_1024] = 1024,
    [ND_2048] = 2048,
    [ND_4096] = 4096,
};

static const float aperture_numbers[] = {
    [AP_1] = 1.0,
    [AP_1_4] = 1.4,
    [AP_2] = 2.0,
    [AP_2_8] = 2.8,
    [AP_4] = 4.0,
    [AP_5_6] = 5.6,
    [AP_8] = 8,
    [AP_11] = 11,
    [AP_16] = 16,
    [AP_22] = 22,
    [AP_32] = 32,
    [AP_45] = 45,
    [AP_64] = 64,
    [AP_90] = 90,
    [AP_128] = 128,
};

static const float time_numbers[] = {
    [TIME_8000] = 1.0 / 8000, [TIME_4000] = 1.0 / 4000, [TIME_2000] = 1.0 / 2000,
    [TIME_1000] = 1.0 / 1000, [TIME_500] = 1.0 / 500,   [TIME_250] = 1.0 / 250,
    [TIME_125] = 1.0 / 125,   [TIME_60] = 1.0 / 60,     [TIME_30] = 1.0 / 30,
    [TIME_15] = 1.0 / 15,     [TIME_8] = 1.0 / 8,       [TIME_4] = 1.0 / 4,
    [TIME_2] = 1.0 / 2,       [TIME_1S] = 1.0,          [TIME_2S] = 2.0,
    [TIME_4S] = 4.0,          [TIME_8S] = 8.0,          [TIME_15S] = 15.0,
    [TIME_30S] = 30.0,
};

struct MainView {
    View* view;
    LightMeterMainViewButtonCallback cb_left;
    void* cb_context;
};

typedef enum {
    FIXED_APERTURE,
    FIXED_TIME,

    /* Know menu Size*/
    MODES_SIZE
} MainViewMode;

typedef struct {
    uint8_t recv[2];
    MainViewMode current_mode;
    int iso;
    int nd;
    int aperture;
    int speed;
} MainViewModel;

void lightmeter_main_view_set_left_callback(
    MainView* lightmeter_main_view,
    LightMeterMainViewButtonCallback callback,
    void* context) {
    with_view_model(
        lightmeter_main_view->view,
        MainViewModel * model,
        {
            UNUSED(model);
            lightmeter_main_view->cb_left = callback;
            lightmeter_main_view->cb_context = context;
        },
        true);
}

static void main_view_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    MainViewModel* model = context;

    canvas_clear(canvas);

    char str[12];
    float EV = 0;
    float A = 0;
    float T = 0;
    int iso = 0;
    float lux;
    bool response = 0;

    if(bh1750_trigger_manual_conversion() == BH1750_OK) response = 1;

    if(response) {
        furi_delay_ms(120);
        bh1750_read_light(&lux);
        EV = lux2ev((float)lux);
        A = aperture_numbers[model->aperture];
        iso = iso_numbers[model->iso];
        T = time_numbers[model->speed];

        float ISO_ND = 0;
        if(model->nd > 0)
            ISO_ND = iso / nd_numbers[model->nd];
        else
            ISO_ND = iso;

        if(lux > 0) {
            if(model->current_mode == FIXED_APERTURE) {
                T = 100 * pow(A, 2) / (double)ISO_ND / pow(2, EV);
            } else if(model->current_mode == FIXED_TIME) {
                A = sqrt(pow(2, EV) * (double)ISO_ND * (double)T / 100);
            }
        } else {
            T = 0;
            A = 0;
        }
    }

    // top row
    if(!response) {
        canvas_draw_box(canvas, 0, 0, 128, 12);
        canvas_set_color(canvas, ColorWhite);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 24, 10, "No sensor found");
        canvas_set_color(canvas, ColorBlack);
    } else {
        canvas_draw_line(canvas, 0, 10, 128, 10);

        canvas_set_font(canvas, FontPrimary);
        // metering mode A – ambient, F – flash
        canvas_draw_str_aligned(canvas, 1, 1, AlignLeft, AlignTop, "A");

        snprintf(str, sizeof(str), "ISO: %d", iso_numbers[model->iso]);
        canvas_draw_str_aligned(canvas, 19, 1, AlignLeft, AlignTop, str);

        canvas_set_font(canvas, FontSecondary);
        snprintf(str, sizeof(str), "lx: %.0f", (double)lux);
        canvas_draw_str_aligned(canvas, 87, 2, AlignLeft, AlignTop, str);
    }

    // add f, T values
    if(model->current_mode == FIXED_APERTURE) {
        canvas_set_font(canvas, FontBigNumbers);

        // draw f icon
        canvas_draw_icon(canvas, 15, 17, &I_f_10x14);

        // draw f number
        if(response) {
            if(model->aperture < AP_8) {
                snprintf(str, sizeof(str), "/%.1f", (double)aperture_numbers[model->aperture]);
            } else {
                snprintf(str, sizeof(str), "/%.0f", (double)aperture_numbers[model->aperture]);
            }
        } else {
            snprintf(str, sizeof(str), " ---");
        }
        canvas_draw_str_aligned(canvas, 27, 15, AlignLeft, AlignTop, str);

        // draw T icon
        canvas_draw_icon(canvas, 15, 34, &I_T_10x14);

        // draw T number
        if(lux > 0 && response) {
            if(T < 1) {
                snprintf(str, sizeof(str), ":1/%.0f", 1 / (double)normalizeTime(T));
            } else {
                snprintf(str, sizeof(str), ":%.0f", (double)normalizeTime(T));
            }
        } else {
            snprintf(str, sizeof(str), " ---");
        }
        canvas_draw_str_aligned(canvas, 27, 34, AlignLeft, AlignTop, str);

    } else if(model->current_mode == FIXED_TIME) {
        canvas_set_font(canvas, FontBigNumbers);

        // draw f icon
        canvas_draw_icon(canvas, 15, 17, &I_f_10x14);

        // draw f number
        if(A < aperture_numbers[0] || !response) {
            snprintf(str, sizeof(str), " ---");
        } else if(A < aperture_numbers[AP_8]) {
            snprintf(str, sizeof(str), "/%.1f", (double)normalizeAperture(A));
        } else {
            snprintf(str, sizeof(str), "/%.0f", (double)normalizeAperture(A));
        }
        canvas_draw_str_aligned(canvas, 27, 15, AlignLeft, AlignTop, str);

        // draw T icon
        canvas_draw_icon(canvas, 15, 34, &I_T_10x14);

        // draw T number
        if(response) {
            if(model->speed < TIME_1S) {
                snprintf(str, sizeof(str), ":1/%.0f", 1 / (double)time_numbers[model->speed]);
            } else {
                snprintf(str, sizeof(str), ":%.0f", (double)time_numbers[model->speed]);
            }
        } else {
            snprintf(str, sizeof(str), " ---");
        }
        canvas_draw_str_aligned(canvas, 27, 34, AlignLeft, AlignTop, str);
    }

    // draw button
    canvas_set_font(canvas, FontSecondary);
    elements_button_left(canvas, "Config");

    // draw ND number
    if(response) {
        snprintf(str, sizeof(str), "ND: %d", nd_numbers[model->nd]);
    } else {
        snprintf(str, sizeof(str), "ND: ---");
    }
    canvas_draw_str_aligned(canvas, 87, 20, AlignLeft, AlignBottom, str);

    // draw EV number
    canvas_set_font(canvas, FontSecondary);
    if(lux > 0 && response) {
        snprintf(str, sizeof(str), "EV: %1.0f", (double)EV);
        canvas_draw_str_aligned(canvas, 87, 29, AlignLeft, AlignBottom, str);
    } else {
        canvas_draw_str_aligned(canvas, 87, 29, AlignLeft, AlignBottom, "EV: --");
    }

    // draw mode indicator
    switch(model->current_mode) {
    case FIXED_TIME:
        canvas_set_font(canvas, FontBigNumbers);
        canvas_draw_str_aligned(canvas, 3, 36, AlignLeft, AlignTop, "*");
        break;

    case FIXED_APERTURE:
        canvas_set_font(canvas, FontBigNumbers);
        canvas_draw_str_aligned(canvas, 3, 17, AlignLeft, AlignTop, "*");
        break;

    default:
        break;
    }
}

static void main_view_process(MainView* main_view, InputEvent* event) {
    with_view_model(
        main_view->view,
        MainViewModel * model,
        {
            if(event->type == InputTypePress) {
                if(event->key == InputKeyUp) {
                    if(model->current_mode == FIXED_APERTURE) {
                        if(model->aperture < AP_NUM - 1) {
                            model->aperture++;
                        }
                    } else if(model->current_mode == FIXED_TIME) {
                        if(model->speed < TIME_NUM - 1) {
                            model->speed++;
                        }
                    }
                } else if(event->key == InputKeyDown) {
                    if(model->current_mode == FIXED_APERTURE) {
                        if(model->aperture > 0) {
                            model->aperture--;
                        }
                    } else if(model->current_mode == FIXED_TIME) {
                        if(model->speed > 0) {
                            model->speed--;
                        }
                    }
                } else if(event->key == InputKeyLeft) {
                } else if(event->key == InputKeyRight) {
                } else if(event->key == InputKeyOk) {
                    if(model->current_mode == FIXED_TIME) {
                        model->current_mode = FIXED_APERTURE;
                    } else if(model->current_mode == FIXED_APERTURE) {
                        model->current_mode = FIXED_TIME;
                    }
                } else if(event->key == InputKeyBack) {
                }
            } else if(event->type == InputTypeRelease) {
                if(event->key == InputKeyUp) {
                } else if(event->key == InputKeyDown) {
                } else if(event->key == InputKeyLeft) {
                } else if(event->key == InputKeyRight) {
                } else if(event->key == InputKeyOk) {
                } else if(event->key == InputKeyBack) {
                }
            } else if(event->type == InputTypeShort) {
                if(event->key == InputKeyBack) {
                }
            }
        },
        true);
}

static bool main_view_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    MainView* main_view = context;
    bool consumed = false;

    if(event->type == InputTypeShort && event->key == InputKeyLeft) {
        if(main_view->cb_left) {
            main_view->cb_left(main_view->cb_context);
        }
        consumed = true;
    } else if(event->type == InputTypeShort && event->key == InputKeyBack) {
    } else {
        main_view_process(main_view, event);
        consumed = true;
    }

    return consumed;
}

MainView* main_view_alloc() {
    MainView* main_view = malloc(sizeof(MainView));
    main_view->view = view_alloc();
    view_set_context(main_view->view, main_view);
    view_allocate_model(main_view->view, ViewModelTypeLocking, sizeof(MainViewModel));
    view_set_draw_callback(main_view->view, main_view_draw_callback);
    view_set_input_callback(main_view->view, main_view_input_callback);

    return main_view;
}

void main_view_free(MainView* main_view) {
    furi_assert(main_view);
    view_free(main_view->view);
    free(main_view);
}

View* main_view_get_view(MainView* main_view) {
    furi_assert(main_view);
    return main_view->view;
}

void main_view_set_iso(MainView* main_view, int iso) {
    furi_assert(main_view);
    with_view_model(
        main_view->view, MainViewModel * model, { model->iso = iso; }, true);
}

void main_view_set_nd(MainView* main_view, int nd) {
    furi_assert(main_view);
    with_view_model(
        main_view->view, MainViewModel * model, { model->nd = nd; }, true);
}

void main_view_set_aperture(MainView* main_view, int aperture) {
    furi_assert(main_view);
    with_view_model(
        main_view->view, MainViewModel * model, { model->aperture = aperture; }, true);
}

void main_view_set_speed(MainView* main_view, int speed) {
    furi_assert(main_view);
    with_view_model(
        main_view->view, MainViewModel * model, { model->speed = speed; }, true);
}
