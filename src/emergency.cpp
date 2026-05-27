#include "emergency.h"
#include <Arduino.h>
#include "config.h"
#include "firebase_helper.h" // Needed to update path and access Database/aClient

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

// Define global localLimits
SensorLimits localLimits;

void checkEmergencyAndNotify(float temperature, float humidity) {
    bool emergency = false;
    float emergencyValue = 0;
    const char* emergencyType = "";

    if (temperature > localLimits.temp_max) {
        emergency = true;
        emergencyValue = temperature;
        emergencyType = "temperature";
        Serial.print(F("EMERGENCY: Temperature "));
        Serial.print(temperature, 1);
        Serial.print(F(" exceeds limit "));
        Serial.println(localLimits.temp_max, 1);
    }

    if (humidity > localLimits.hum_max) {
        emergency = true;
        emergencyValue = humidity;
        emergencyType = "humidity";
        Serial.print(F("EMERGENCY: Humidity "));
        Serial.print(humidity, 1);
        Serial.print(F(" exceeds limit "));
        Serial.println(localLimits.hum_max, 1);
    }

    if (emergency) {
        char payload[192];
        snprintf(payload, sizeof(payload),
                 "{\"emergency\":true,\"emergencyType\":\"%s\",\"emergencyValue\":%.1f,\"temp_max\":%.1f,\"hum_max\":%.1f,\"timestamp\":%lu}",
                 emergencyType, emergencyValue, localLimits.temp_max, localLimits.hum_max, millis());

        Database.update(aClient, FIREBASE_PATH_TELEMETRY, object_t(payload), databaseResult);

        sendEmergencyToVercel(temperature, humidity);
    }
}

void sendEmergencyToVercel(float temperature, float humidity) {
#if defined(ESP8266)
    WiFiClient client;
    HTTPClient http;
    http.begin(client, VERCEL_WEBHOOK_URL);
    http.addHeader("Content-Type", "application/json");

    char payload[192];
    snprintf(payload, sizeof(payload),
             "{\"deviceId\":\"%s\",\"plantId\":\"%s\",\"temperature\":%.1f,\"humidity\":%.1f,\"temp_max\":%.1f,\"hum_max\":%.1f,\"emergency\":true}",
             CONFIG_DEVICE_ID, CONFIG_PLANT_ID, temperature, humidity, localLimits.temp_max, localLimits.hum_max);

    int httpCode = http.POST((uint8_t*)payload, strlen(payload));
    Serial.print(F("Vercel webhook response code: "));
    Serial.println(httpCode);
    http.end();
#endif
}
