#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_serial.h>
#include <furi_hal_serial_control.h>
#include <gui/gui.h>
#include <input/input.h>

#include "navigation2.h"
#include "uart_handler.h"
#include "bw16.h"
#include "draw.h"

typedef struct {
    Navigation2 nav;
    UartHandler uart;
    FuriMutex* mutex;
    FuriThread* thread;
    FuriHalSerialHandle* serial;
    FuriMessageQueue* event_queue;
} App;

// Input callback pour navigation
static void input_callback(InputEvent* event, void* ctx) {
    App* app = ctx;
    furi_message_queue_put(app->event_queue, event, FuriWaitForever);
}

// Thread principal de l'application
static int32_t app_thread_callback(void* ctx) {
    App* app = ctx;
    InputEvent event;

    while(navigation2_running(&app->nav)) {
        // Wait for event or timeout for update loop
        FuriStatus status = furi_message_queue_get(app->event_queue, &event, 50);

        furi_mutex_acquire(app->mutex, FuriWaitForever);

        if(status == FuriStatusOk) {
            navigation2_input(&app->nav, &event);
        }

        navigation2_update(&app->nav, &app->uart);

        furi_mutex_release(app->mutex);
    }

    return 0;
}

// Callback UART réception
static void uart_rx_callback(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* ctx) {
    UartHandler* handler = ctx;
    if(event & FuriHalSerialRxEventData) {
        uint8_t data = furi_hal_serial_async_rx(handle);
        uart_handler_push(handler, data);
    }
}

int32_t app_5g_rebuild(void* p) {
    UNUSED(p);

    // Allocation App
    App* app = malloc(sizeof(App));
    if(!app) return 255;

    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Init Navigation
    navigation2_init(&app->nav);

    // Init UART
    uart_handler_init(&app->uart);

    // Open UART
    app->serial = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    if(!app->serial) {
        FURI_LOG_E("5G", "UART open failed");
        furi_mutex_free(app->mutex);
        free(app);
        return 255;
    }

    // Config callback UART et démarrer réception async
    furi_hal_serial_init(app->serial, 115200);
    furi_hal_serial_async_rx_start(app->serial, uart_rx_callback, &app->uart, false);

    // Création thread principal
    app->thread = furi_thread_alloc();
    furi_thread_set_name(app->thread, "5GRebuildThread");
    furi_thread_set_stack_size(app->thread, 2048);
    furi_thread_set_context(app->thread, app);
    furi_thread_set_callback(app->thread, app_thread_callback);
    furi_thread_start(app->thread);

    // GUI
    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, draw_callback, app);
    view_port_input_callback_set(vp, input_callback, app);

    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    // Boucle principale
    while(navigation2_running(&app->nav)) {
        furi_delay_ms(50);
    }

    // Nettoyage GUI
    gui_remove_view_port(gui, vp);
    furi_record_close("gui");
    view_port_free(vp);

    // Terminer thread
    furi_thread_join(app->thread);
    furi_thread_free(app->thread);

    // Fermer UART
    furi_hal_serial_deinit(app->serial);
    furi_hal_serial_control_release(app->serial);

    // Libération mutex et mémoire
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->mutex);
    free(app);

    return 0;
}
