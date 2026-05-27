#include "wifi_helper.h"
#include <Arduino.h>
#include "config.h"
#include "ap_provisioning.h"

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

void connectWiFi() {
    String ssid = "";
    String password = "";
    
    getWiFiCredentials(ssid, password);

    if (ssid == "") {
        Serial.println(F("No WiFi credentials found. Starting AP Mode."));
        initAPProvisioning();
        return;
    }

    Serial.println(F("Connecting to WiFi: "));
    Serial.println(ssid);
    
    // Explicitly turn off AP mode
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT_MS) {
        Serial.print(F("."));
        delay(300);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println(F("WiFi connected!"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println(F("WiFi connection FAILED. Will retry..."));
    }
}

void verifyWiFi() {
    if (WiFi.status() != WL_CONNECTED && !apProvisioningActive) {
        String ssid = "";
        String password = "";
        getWiFiCredentials(ssid, password);
        
        if(ssid != "") {
            Serial.println(F("WiFi disconnected. Reconnecting..."));
            WiFi.reconnect();
            delay(1000);
        }
    }
}
