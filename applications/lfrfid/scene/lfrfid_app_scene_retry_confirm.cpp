#include "lfrfid_app_scene_retry_confirm.h"
#include "../view/elements/button_element.h"
#include "../view/elements/icon_element.h"
#include "../view/elements/string_element.h"

void LfRfidAppSceneRetryConfirm::on_enter(LfRfidApp* app, bool /* need_restore */) {
    auto container = app->view_controller.get<ContainerVM>();

    auto button = container->add<ButtonElement>();
    button->set_type(ButtonElement::Type::Left, "Exit");
    button->set_callback(app, LfRfidAppSceneRetryConfirm::exit_callback);

    button = container->add<ButtonElement>();
    button->set_type(ButtonElement::Type::Right, "Stay");
    button->set_callback(app, LfRfidAppSceneRetryConfirm::stay_callback);

    auto line_1 = container->add<StringElement>();
    auto line_2 = container->add<StringElement>();

    line_1->set_text("Return to Reading?", 64, 19, 128 - 2, AlignCenter, AlignBottom, FontPrimary);
    line_2->set_text(
        "All unsaved data will be lost!", 64, 29, 0, AlignCenter, AlignBottom, FontSecondary);

    app->view_controller.switch_to<ContainerVM>();
}

bool LfRfidAppSceneRetryConfirm::on_event(LfRfidApp* app, LfRfidApp::Event* event) {
    bool consumed = false;

    if(event->type == LfRfidApp::EventType::Next) {
        app->scene_controller.search_and_switch_to_previous_scene({LfRfidApp::SceneType::Read});
        consumed = true;
    } else if(event->type == LfRfidApp::EventType::Stay) {
        app->scene_controller.switch_to_previous_scene();
        consumed = true;
    } else if(event->type == LfRfidApp::EventType::Back) {
        consumed = true;
    }

    return consumed;
}

void LfRfidAppSceneRetryConfirm::on_exit(LfRfidApp* app) {
    app->view_controller.get<ContainerVM>()->clean();
}

void LfRfidAppSceneRetryConfirm::exit_callback(void* context) {
    LfRfidApp* app = static_cast<LfRfidApp*>(context);
    LfRfidApp::Event event;
    event.type = LfRfidApp::EventType::Next;
    app->view_controller.send_event(&event);
}

void LfRfidAppSceneRetryConfirm::stay_callback(void* context) {
    LfRfidApp* app = static_cast<LfRfidApp*>(context);
    LfRfidApp::Event event;
    event.type = LfRfidApp::EventType::Stay;
    app->view_controller.send_event(&event);
}
