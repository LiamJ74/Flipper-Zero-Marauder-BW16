# WIP

# Flipper Zero 5G Rebuild (BW16 Marauder)

This application is a dedicated Flipper Zero firmware app designed to control a **BW16 (RTL8720DN)** WiFi module. It allows scanning for WiFi networks (2.4GHz and 5GHz) and performing deauthentication attacks.

## 🚀 Features

*   **WiFi Scanning:** Scans for nearby access points using the BW16 module.
*   **Dual Band:** Supports 5GHz and 2.4GHz (dependent on BW16 firmware capabilities).
*   **RSSI Sorting:** Automatically displays the top 10 strongest networks.
*   **Deauth Attack:** Send deauthentication frames to a selected target.
*   **Robust UI:** Custom list navigation with scrolling and status updates.

## 🛠 Wiring (Flipper Zero <-> BW16)

This application uses the Flipper Zero's expansion port (USART).

| Flipper Zero Pin | Flipper Function | BW16 Pin | Description |
| :--- | :--- | :--- | :--- |
| **Pin 13 (PC10)** | **TX** | **RX** | Data from Flipper to BW16 |
| **Pin 14 (PC11)** | **RX** | **TX** | Data from BW16 to Flipper |
| **GND (Pin 8/18)** | **GND** | **GND** | Ground common |
| **3V3 (Pin 9)** | **3V3** | **3V3/VCC**| Power Supply |

> **Note:** Ensure your BW16 module is flashed with the compatible "Marauder" firmware that outputs JSON scan results over UART at **115200 baud**.

## 📦 Building

To compile this application as a `.fap` (Flipper Application Package):

1.  Place this folder in `applications_user/`.
2.  Run the FBT (Flipper Build Tool) command:

```bash
./fbt fap_5g_rebuild
```

3.  The resulting file will be in `build/f7-firmware-D/.extapps/5g_rebuild.fap`.

## 🎮 Usage

1.  **Launch:** Open the app from the Flipper Zero "Apps" menu.
2.  **Main Menu:**
    *   **Scan WiFi:** Starts a new scan. The screen will show "Scanning..." for ~10 seconds.
    *   **Last Scan:** Retries fetching the last scan result from the module.
3.  **Network List:**
    *   Use **Up/Down** to scroll through the list of networks.
    *   Press **OK** to select a target.
4.  **Target Options:**
    *   Select **Deauth** to launch an attack on the selected network.

## ⚠️ Disclaimer

This tool is for educational purposes and authorized security testing only. Using this tool against networks without permission is illegal. The authors differ all responsibility for misuse.
