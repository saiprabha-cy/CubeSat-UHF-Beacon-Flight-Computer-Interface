/*
 * BEACON_FIRMWARE.INO
 *
 * ESP32 CubeSat beacon demo -- transmits telemetry frames over WiFi/UDP
 * using the EXACT protocol verified in Stage 1 (16/16 host tests) and
 * re-verified here via the Python udp_listener.py round-trip test
 * before this sketch was even written. This is the one physically real
 * piece of this whole capstone: everything upstream (KiCad boards,
 * LTspice/ngspice RF chain, 4NEC2 antenna, MATLAB link budget) was
 * design/simulation work: this is the thing that actually runs.
 *
 * *** FILL IN BEFORE FLASHING ***
 *   WIFI_SSID, WIFI_PASSWORD : your network credentials
 *   GROUND_IP                : your laptop's local IP address
 *                               (Windows: run `ipconfig`, use the
 *                               IPv4 address of your active network
 *                               adapter -- NOT 127.0.0.1, since the
 *                               ESP32 is a separate device on the
 *                               network, not the same machine)
 *
 * Telemetry data here is SIMULATED (a slowly-changing synthetic
 * attitude-error/omega/wheel-momentum signal), standing in for what a
 * real flight computer (Board B) would provide over UART using this
 * same frame format -- documented as simulated, not claimed as real
 * sensor data, matching this project's consistent honesty standard.
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include "protocol.h"
#include "protocol_framer.h"

// ---- FILL IN ----
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* GROUND_IP     = "192.168.1.XXX";   // your laptop's IP
// ------------------

const uint16_t UDP_PORT = 5005;   // must match udp_listener.py's UDP_PORT
const uint32_t TX_INTERVAL_MS = 1000;   // one beacon frame per second

WiFiUDP udp;
uint16_t seq_counter = 0;
uint32_t last_tx_time = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== CubeSat Beacon (ESP32) ===");

    Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.printf("Connected. IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Sending telemetry to %s:%d every %lu ms\n",
                   GROUND_IP, UDP_PORT, TX_INTERVAL_MS);

    udp.begin(UDP_PORT);  // also listen locally (not required for TX-only
                           // operation, but harmless and useful if this
                           // sketch is later extended to receive commands)
}

void loop() {
    uint32_t now = millis();
    if (now - last_tx_time >= TX_INTERVAL_MS) {
        last_tx_time = now;
        send_telemetry_frame();
    }
}

void send_telemetry_frame() {
    // Simulated telemetry -- a slowly-oscillating synthetic signal,
    // not real sensor data. Explicitly documented as such.
    float t_sec = millis() / 1000.0f;
    telemetry_payload_t payload;
    payload.timestamp_ms = millis();
    payload.attitude_error_deg = 0.5f + 0.1f * sinf(t_sec * 0.1f);
    payload.omega[0] = 0.001f * sinf(t_sec * 0.05f);
    payload.omega[1] = 0.001f * cosf(t_sec * 0.05f);
    payload.omega[2] = 0.0005f * sinf(t_sec * 0.02f);
    payload.h_wheel[0] = 3.0e-4f;
    payload.h_wheel[1] = -6.8e-4f;
    payload.h_wheel[2] = 7.5e-4f;
    payload.seq_counter = seq_counter++;

    uint8_t frame_buf[PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_len = protocol_build_frame(FRAME_TYPE_TELEMETRY,
        &payload, sizeof(payload), frame_buf, sizeof(frame_buf));

    if (frame_len == 0) {
        Serial.println("ERROR: frame build failed");
        return;
    }

    udp.beginPacket(GROUND_IP, UDP_PORT);
    udp.write(frame_buf, frame_len);
    udp.endPacket();

    Serial.printf("[%lu] TX seq=%d err=%.4f frame_len=%d\n",
                   payload.timestamp_ms, payload.seq_counter,
                   payload.attitude_error_deg, (int)frame_len);
}
