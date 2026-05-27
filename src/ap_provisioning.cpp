#include "ap_provisioning.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);
bool apProvisioningActive = false;

void getWiFiCredentials(String& outSsid, String& outPassword) {
    if (!hasWiFiCredentials()) return;
    
    Credentials creds;
    EEPROM.begin(sizeof(Credentials) + 1);
    EEPROM.get(1, creds);
    EEPROM.end();
    
    outSsid = String(creds.ssid);
    outPassword = String(creds.password);
}

void handleOptions() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
}

void handleSetup() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    if (server.method() == HTTP_OPTIONS) {
        handleOptions();
        return;
    }

    if (server.hasArg("plain") == false) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Body not received\"}");
        return;
    }

    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    const char* ssid = doc["ssid"];
    const char* password = doc["password"];

    if (ssid && password && strlen(ssid) > 0) {
        Credentials creds;
        memset(&creds, 0, sizeof(Credentials));
        strncpy(creds.ssid, ssid, sizeof(creds.ssid) - 1);
        strncpy(creds.password, password, sizeof(creds.password) - 1);

        EEPROM.begin(sizeof(Credentials) + 1); // +1 for valid flag
        EEPROM.write(0, 0xAA); // Validation flag
        EEPROM.put(1, creds);
        EEPROM.commit();
        EEPROM.end();

        server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Credentials saved. Rebooting.\"}");
        Serial.println(F("Credentials saved to EEPROM. Rebooting in 2s..."));
        
        delay(2000);
        ESP.restart();
    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID or Password\"}");
    }
}

void initAPProvisioning() {
    if (apProvisioningActive) return;

    Serial.println(F("Starting AP Provisioning..."));
    
    // Setup SSID as MAC address
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String apName = "Sensor_" + mac;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str());

    Serial.print(F("AP Started. SSID: "));
    Serial.println(apName);
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.softAPIP());

    server.on("/setup", HTTP_POST, handleSetup);
    server.on("/setup", HTTP_OPTIONS, handleOptions);
    server.begin();

    apProvisioningActive = true;
}

void handleAPClient() {
    if (apProvisioningActive) {
        server.handleClient();
    }
}

void clearWiFiCredentials() {
    EEPROM.begin(sizeof(Credentials) + 1);
    EEPROM.write(0, 0x00); // Clear validation flag
    EEPROM.commit();
    EEPROM.end();
    Serial.println(F("WiFi credentials cleared from EEPROM."));
}

bool hasWiFiCredentials() {
    EEPROM.begin(sizeof(Credentials) + 1);
    byte flag = EEPROM.read(0);
    EEPROM.end();
    return flag == 0xAA;
}
