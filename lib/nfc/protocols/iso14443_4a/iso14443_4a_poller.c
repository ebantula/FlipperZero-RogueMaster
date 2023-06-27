#include "iso14443_4a_poller_i.h"

#include <nfc/protocols/nfc_poller_base.h>

#include <furi.h>

#define TAG "Iso14443_4aPoller"

#define ISO14443_4A_BUF_SIZE_MAX (512U)

typedef NfcCommand (*Iso14443_4aPollerStateHandler)(Iso14443_4aPoller* instance);

const Iso14443_4aData* iso14443_4a_poller_get_data(Iso14443_4aPoller* instance) {
    furi_assert(instance);

    return instance->data;
}

static Iso14443_4aPoller* iso14443_4a_poller_alloc(NfcaPoller* iso14443_3a_poller) {
    Iso14443_4aPoller* instance = malloc(sizeof(Iso14443_4aPoller));
    instance->iso14443_3a_poller = iso14443_3a_poller;
    instance->data = iso14443_4a_alloc();
    instance->tx_buffer = bit_buffer_alloc(ISO14443_4A_BUF_SIZE_MAX);
    instance->rx_buffer = bit_buffer_alloc(ISO14443_4A_BUF_SIZE_MAX);

    instance->iso14443_4a_event.data = &instance->iso14443_4a_event_data;

    instance->general_event.protocol_type = NfcProtocolTypeIso14443_4a;
    instance->general_event.data = &instance->iso14443_4a_event;
    instance->general_event.poller = instance;

    return instance;
}

static void iso14443_4a_poller_free(Iso14443_4aPoller* instance) {
    furi_assert(instance);

    iso14443_4a_free(instance->data);
    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->rx_buffer);
    free(instance);
}

static NfcCommand iso14443_4a_poller_handler_idle(Iso14443_4aPoller* instance) {
    nfca_copy(
        instance->data->iso14443_3a_data, nfca_poller_get_data(instance->iso14443_3a_poller));

    instance->poller_state = Iso14443_4aPollerStateReadAts;
    instance->protocol_state.block_number = 0;
    return NfcCommandContinue;
}

static NfcCommand iso14443_4a_poller_handler_read_ats(Iso14443_4aPoller* instance) {
    Iso14443_4aError error =
        iso14443_4a_poller_async_read_ats(instance, &instance->data->ats_data);
    if(error == Iso14443_4aErrorNone) {
        FURI_LOG_D(TAG, "Read ATS success");
        instance->poller_state = Iso14443_4aPollerStateReady;
    } else {
        FURI_LOG_D(TAG, "Failed to read ATS");
        instance->poller_state = Iso14443_4aPollerStateError;
    }

    return NfcCommandContinue;
}

static NfcCommand iso14443_4a_poller_handler_error(Iso14443_4aPoller* instance) {
    nfca_poller_halt(instance->iso14443_3a_poller);
    instance->iso14443_4a_event_data.error = instance->error;
    NfcCommand command = instance->callback(instance->general_event, instance->context);
    instance->poller_state = Iso14443_4aPollerStateIdle;
    return command;
}

static NfcCommand iso14443_4a_poller_handler_ready(Iso14443_4aPoller* instance) {
    instance->iso14443_4a_event.type = Iso14443_4aPollerEventTypeReady;
    NfcCommand command = instance->callback(instance->general_event, instance->context);
    return command;
}

static const Iso14443_4aPollerStateHandler
    iso14443_4a_poller_state_handler[Iso14443_4aPollerStateNum] = {
        [Iso14443_4aPollerStateIdle] = iso14443_4a_poller_handler_idle,
        [Iso14443_4aPollerStateReadAts] = iso14443_4a_poller_handler_read_ats,
        [Iso14443_4aPollerStateError] = iso14443_4a_poller_handler_error,
        [Iso14443_4aPollerStateReady] = iso14443_4a_poller_handler_ready,
};

static void iso14443_4a_poller_set_callback(
    Iso14443_4aPoller* instance,
    NfcPollerCallback callback,
    void* context) {
    furi_assert(instance);
    furi_assert(callback);

    instance->callback = callback;
    instance->context = context;
}

static NfcCommand iso14443_4a_poller_run(NfcPollerEvent event, void* context) {
    furi_assert(event.protocol_type == NfcProtocolTypeIso14443_3a);

    Iso14443_4aPoller* instance = context;
    furi_assert(instance);
    furi_assert(instance->callback);

    NfcaPollerEvent* iso14443_3a_event = event.data;
    furi_assert(iso14443_3a_event);

    NfcCommand command = NfcCommandContinue;

    if(iso14443_3a_event->type == NfcaPollerEventTypeReady) {
        command = iso14443_4a_poller_state_handler[instance->poller_state](instance);
    } else if(iso14443_3a_event->type == NfcaPollerEventTypeError) {
        instance->iso14443_4a_event.type = Iso14443_4aPollerEventTypeError;
        command = instance->callback(instance->general_event, instance->context);
    }

    return command;
}

static bool iso14443_4a_poller_detect(NfcPollerEvent event, void* context) {
    furi_assert(event.protocol_type == NfcProtocolTypeIso14443_3a);

    const Iso14443_4aPoller* instance = context;
    furi_assert(instance);

    const NfcaPollerEvent* iso14443_3a_event = event.data;
    furi_assert(iso14443_3a_event);
    nfca_copy(
        instance->data->iso14443_3a_data, nfca_poller_get_data(instance->iso14443_3a_poller));

    bool protocol_detected = false;

    if(iso14443_3a_event->type == NfcaPollerEventTypeReady) {
        protocol_detected = iso14443_4a_is_ats_supported(instance->data);
    }

    return protocol_detected;
}

const NfcPollerBase nfc_poller_iso14443_4a = {
    .alloc = (NfcPollerAlloc)iso14443_4a_poller_alloc,
    .free = (NfcPollerFree)iso14443_4a_poller_free,
    .set_callback = (NfcPollerSetCallback)iso14443_4a_poller_set_callback,
    .run = (NfcPollerRun)iso14443_4a_poller_run,
    .detect = (NfcPollerDetect)iso14443_4a_poller_detect,
    .get_data = (NfcPollerGetData)iso14443_4a_poller_get_data,
};
