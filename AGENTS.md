# AI Agent Guide for 5g_rebuild

This document provides context, architectural decisions, and constraints for AI agents working on this codebase.

## 🏗 Architecture Overview

The application follows the standard Flipper Zero application pattern but uses a custom state machine for navigation.

1.  **Entry Point (`main.c`):**
    *   Initializes the `App` struct.
    *   Sets up the UART using `furi_hal_serial_control`.
    *   Creates a `FuriMessageQueue` for thread-safe input handling.
    *   Runs the main loop in `app_thread_callback`, ensuring sequential processing of Input -> Logic -> UART.

2.  **Navigation (`navigation2.c` / `.h`):**
    *   Acts as the **Controller** and **Model**.
    *   Manages the state machine: `NavStateMain`, `NavStateScanning`, `NavStateList`, `NavStateOptions`.
    *   Holds the `WifiApp` data (list of networks).
    *   Processes `InputEvent`s to transition states.
    *   Processes UART data to populate the network list.

3.  **UI / View (`draw.c`):**
    *   Stateless rendering callback.
    *   Casts the `ctx` pointer to `Navigation2*` to read the current state.
    *   Renders the screen based on `nav->state`.

4.  **Hardware / Driver (`bw16.c`, `uart_handler.c`):**
    *   `bw16.c`: Helper functions to construct and send raw bytes to the BW16 module.
    *   `uart_handler.c`: Legacy ring buffer implementation (partially superseded by `json_parser` but still used for buffering).
    *   `json_parser.c`: Parses incoming UART JSON streams into `WifiNetwork` structs.

## 🔑 Key Constraints & patterns

*   **UART API:** Do **not** use `furi_hal_serial_open`. You **must** use `furi_hal_serial_control_acquire(FuriHalSerialIdUsart)` to respect the expansion port ownership.
*   **Threading:** The `input_callback` runs in the GUI thread. Do **not** modify state or call UART functions directly from it. Use `furi_message_queue_put` with a timeout of `0`.
*   **Standard Library:** The Flipper Zero firmware does not link `qsort` for external apps. Use local sorting implementations (e.g., bubble sort) if needed.
*   **Entry Point:** The function name in `main.c` (e.g., `int32_t app_5g_rebuild(void* p)`) **must** match the `entry_point` defined in `application.fam`. Failure to match causes linker errors (`undefined symbol root`).
*   **Strings:** `snprintf` is strict. Always ensure buffer sizes are sufficient to prevent truncation warnings, which are treated as errors (`-Werror`).

## 📂 File Responsibilities

| File | Purpose |
| :--- | :--- |
| `main.c` | App lifecycle, thread management, UART init/deinit. |
| `navigation2.c` | Core logic, state transitions, sorting, input handling. |
| `draw.c` | Drawing logic for the ViewPort. |
| `bw16.c` | Protocol commands (Scan, Deauth) for the BW16 module. |
| `json_parser.c` | Parsing JSON responses from the module (robust `strstr` usage). |
| `application.fam` | Build manifest. Defines ID, Name, Entry Point, and Sources. |

## 🧪 Common Tasks

*   **Adding a Menu Item:**
    1.  Update `draw.c` to render the new item in the relevant state.
    2.  Update `navigation2.c` (`navigation2_input`) to handle the selection index and trigger the action.
*   **Handling New UART Data:**
    1.  Update `json_parser.c` to parse the new field.
    2.  Update `wifi_types.h` struct if necessary.
