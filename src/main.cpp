#include <Arduino.h>
#include "config.h"
#include "wifi_helper.h"
#include "firebase_helper.h"
#include "ap_provisioning.h"

#define BOOT_BUTTON_PIN 0

unsigned long lastSend = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println(F("==========================================="));
    Serial.println(F("  ESP32 Firebase Telemetry v1.0"));
    Serial.println(F("  Industrial Plant Monitoring"));
    Serial.println(F("  Smart Edge IoT Architecture"));
    Serial.println(F("==========================================="));
    Serial.println();

    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // Connect to WiFi if configured
    if (hasWiFiCredentials()) {
        connectWiFi();
        // Initialize Firebase
        initFirebase();
        // Initiate real-time subscription
        fetchLimits();
    } else {
        Serial.println(F("Device unconfigured. Waiting for BOOT button press to start AP Mode."));
    }
}

void loop() {
    static unsigned long buttonPressTime = 0;
    static bool buttonPressed = false;

    // BOOT Button Logic
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        if (!buttonPressed) {
            buttonPressTime = millis();
            buttonPressed = true;
        }
    } else {
        if (buttonPressed) {
            unsigned long duration = millis() - buttonPressTime;
            buttonPressed = false;
            
            if (duration > 3000) {
                // Long press: clear credentials
                clearWiFiCredentials();
                ESP.restart();
            } else if (duration > 50) {
                // Short press: start AP Mode
                initAPProvisioning();
            }
        }
    }

    if (apProvisioningActive) {
        handleAPClient();
        delay(10);
        return; 
    }

    if (!hasWiFiCredentials()) {
        delay(10);
        return;
    }

    // Maintain Firebase tasks
    app.loop();

    // Handle async database results
    processFirebaseData();

    // Reconnect WiFi if disconnected
    verifyWiFi();

    // Send telemetry at configured interval
    unsigned long now = millis();
    if (now - lastSend >= TELEMETRY_INTERVAL_MS) {
        if (app.ready())
            sendTelemetry();
        lastSend = now;
    }

    delay(10);
}