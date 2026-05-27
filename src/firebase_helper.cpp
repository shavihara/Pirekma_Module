#include "firebase_helper.h"
#include "config.h"
#include "emergency.h"
#include "ExampleFunctions.h"

// Define global objects
SSL_CLIENT ssl_client;
SSL_CLIENT ssl_client_stream;
AsyncClient aClient(ssl_client);
AsyncClient aClientStream(ssl_client_stream);

UserAuth user_auth(FIREBASE_API_KEY, FIREBASE_AUTH_EMAIL, FIREBASE_AUTH_PASSWORD, 3600);
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult databaseResult;
AsyncResult limitsResult;

// Private helper function declaration
void processData(AsyncResult &aResult);
static bool parseSSEPayload(const char* raw, String &eventType, String &dataPath, String &dataPayload);

void initFirebase() {
    // Configure SSL clients
    ssl_client.setInsecure();
#if defined(ESP8266)
    ssl_client.setBufferSizes(1024, 512); // Optimized for small telemetry payloads
#endif

    ssl_client_stream.setInsecure();
#if defined(ESP8266)
    ssl_client_stream.setBufferSizes(2048, 512); // Optimized for limits streaming
#endif

    // Initialize Firebase app with user auth and debug callback
    initializeApp(aClient, app, getAuth(user_auth), auth_debug_print);

    // Attach RealtimeDatabase instance and set URL
    app.getApp<RealtimeDatabase>(Database);
    Database.url(FIREBASE_DATABASE_URL);

    // Set filters for the stream client (SSE stream)
    aClientStream.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
}

void fetchLimits() {
    Serial.print(F("Subscribing to limits stream on Firebase path: "));
    Serial.println(F(FIREBASE_PATH_LIMITS));
    Database.get(aClientStream, FIREBASE_PATH_LIMITS, limitsResult, true /* SSE mode */);
}

void sendTelemetry() {
    float temperature = random(150, 351) / 10.0;  // 15.0 - 35.0 °C
    float humidity = random(400, 901) / 10.0;      // 40.0 - 90.0 %
    float pressure = random(60, 121) / 10.0;       // 6.0 - 12.0 psi

    Serial.println(F("=== Sending Telemetry ==="));
    Serial.print(F("Free Heap: ")); Serial.print(ESP.getFreeHeap()); Serial.println(F(" bytes"));
    Serial.print(F("Temperature: ")); Serial.print(temperature, 1); Serial.println(F(" °C"));
    Serial.print(F("Humidity: ")); Serial.print(humidity, 1); Serial.println(F(" %"));
    Serial.print(F("Pressure: ")); Serial.print(pressure, 1); Serial.println(F(" psi"));
    Serial.print(F("Local Limits -> temp_max: ")); Serial.print(localLimits.temp_max, 1);
    Serial.print(F(", hum_max: ")); Serial.println(localLimits.hum_max, 1);

    // Check for emergency conditions (Smart Edge logic)
    checkEmergencyAndNotify(temperature, humidity);

    // Format JSON telemetry payload using a stack-allocated buffer (0 heap allocations)
    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"telemetry\":{\"temperature\":%.1f,\"humidity\":%.1f,\"pressure\":%.1f,\"timestamp\":{\".sv\":\"timestamp\"}},\"info\":{\"online\":true,\"lastPing\":{\".sv\":\"timestamp\"}}}",
             temperature, humidity, pressure);

    Database.update(aClient, FIREBASE_PATH_DEVICE, object_t(payload), databaseResult);
}

void processFirebaseData() {
    processData(databaseResult);
    processData(limitsResult);
}

void processData(AsyncResult &aResult) {
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
    {
        Firebase.printf(PSTR("Event task: %s, msg: %s, code: %d\n"), aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf(PSTR("Debug task: %s, msg: %s\n"), aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf(PSTR("Error task: %s, msg: %s, code: %d\n"), aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        Firebase.printf(PSTR("task: %s, payload: %s\n"), aResult.uid().c_str(), aResult.c_str());

        if (&aResult == &limitsResult) {
            RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
            if (RTDB.isStream()) {
                String eventTypeStr, dataPathStr, payloadStr;
                if (parseSSEPayload(aResult.c_str(), eventTypeStr, dataPathStr, payloadStr)) {
                    const char* eventType = eventTypeStr.c_str();
                    const char* dataPath = dataPathStr.c_str();
                    const char* payload = payloadStr.c_str();
                    
                    Serial.print(F("Stream Event: "));
                    Serial.print(eventType);
                    Serial.print(F(", Path: "));
                    Serial.print(dataPath);
                    Serial.print(F(", Payload: "));
                    Serial.println(payload);

                    if (strcmp(eventType, "put") == 0 || strcmp(eventType, "patch") == 0) {
                        if (strcmp(dataPath, "/") == 0 || strcmp(dataPath, "") == 0) {
                            // The payload is the entire limits JSON object
                            ArduinoJson::JsonDocument doc;
                            if (ArduinoJson::DeserializationError::Ok == ArduinoJson::deserializeJson(doc, payload)) {
                                ArduinoJson::JsonObject root = doc.as<ArduinoJson::JsonObject>();
                                
                                // Parse temperature limit (supports temp_max and temperature_max)
                                if (!root["temp_max"].isNull()) {
                                    localLimits.temp_max = root["temp_max"].as<float>();
                                    Serial.print(F("Stream updated temp_max: "));
                                    Serial.println(localLimits.temp_max, 1);
                                } else if (!root["temperature_max"].isNull()) {
                                    localLimits.temp_max = root["temperature_max"].as<float>();
                                    Serial.print(F("Stream updated temp_max (from temperature_max): "));
                                    Serial.println(localLimits.temp_max, 1);
                                }

                                // Parse humidity limit (supports hum_max and humidity_max)
                                if (!root["hum_max"].isNull()) {
                                    localLimits.hum_max = root["hum_max"].as<float>();
                                    Serial.print(F("Stream updated hum_max: "));
                                    Serial.println(localLimits.hum_max, 1);
                                } else if (!root["humidity_max"].isNull()) {
                                    localLimits.hum_max = root["humidity_max"].as<float>();
                                    Serial.print(F("Stream updated hum_max (from humidity_max): "));
                                    Serial.println(localLimits.hum_max, 1);
                                }
                            }
                        } else if (strcmp(dataPath, "/temp_max") == 0 || strcmp(dataPath, "/temperature_max") == 0) {
                            localLimits.temp_max = atof(payload);
                            Serial.print(F("Stream updated temp_max (single): "));
                            Serial.println(localLimits.temp_max, 1);
                        } else if (strcmp(dataPath, "/hum_max") == 0 || strcmp(dataPath, "/humidity_max") == 0) {
                            localLimits.hum_max = atof(payload);
                            Serial.print(F("Stream updated hum_max (single): "));
                            Serial.println(localLimits.hum_max, 1);
                        }
                    }
                }
            } else {
                // One-time GET fallback parsing
                ArduinoJson::JsonDocument doc;
                if (ArduinoJson::DeserializationError::Ok == ArduinoJson::deserializeJson(doc, aResult.c_str())) {
                    ArduinoJson::JsonObject root = doc.as<ArduinoJson::JsonObject>();
                    
                    // Parse temperature limit (supports temp_max and temperature_max)
                    if (!root["temp_max"].isNull()) {
                        localLimits.temp_max = root["temp_max"].as<float>();
                        Serial.print(F("Updated temp_max: "));
                        Serial.println(localLimits.temp_max, 1);
                    } else if (!root["temperature_max"].isNull()) {
                        localLimits.temp_max = root["temperature_max"].as<float>();
                        Serial.print(F("Updated temp_max (from temperature_max): "));
                        Serial.println(localLimits.temp_max, 1);
                    }

                    // Parse humidity limit (supports hum_max and humidity_max)
                    if (!root["hum_max"].isNull()) {
                        localLimits.hum_max = root["hum_max"].as<float>();
                        Serial.print(F("Updated hum_max: "));
                        Serial.println(localLimits.hum_max, 1);
                    } else if (!root["humidity_max"].isNull()) {
                        localLimits.hum_max = root["humidity_max"].as<float>();
                        Serial.print(F("Updated hum_max (from humidity_max): "));
                        Serial.println(localLimits.hum_max, 1);
                    }
                }
            }
        }
    }
}

static bool parseSSEPayload(const char* raw, String &eventType, String &dataPath, String &dataPayload) {
    if (!raw) return false;

    // Find "event:"
    const char* eventPtr = strstr(raw, "event:");
    if (!eventPtr) return false;
    eventPtr += 6; // skip "event:"
    while (*eventPtr == ' ' || *eventPtr == '\t') eventPtr++;

    const char* eventEnd = strchr(eventPtr, '\n');
    if (!eventEnd) return false;
    
    // Trim trailing '\r' or spaces
    const char* tempEnd = eventEnd;
    while (tempEnd > eventPtr && (*(tempEnd - 1) == '\r' || *(tempEnd - 1) == ' ' || *(tempEnd - 1) == '\t')) {
        tempEnd--;
    }
    
    eventType = "";
    while (eventPtr < tempEnd) {
        eventType += *eventPtr++;
    }

    // Find "data:"
    const char* dataPtr = strstr(raw, "data:");
    if (!dataPtr) return false;
    dataPtr += 5; // skip "data:"
    while (*dataPtr == ' ' || *dataPtr == '\t') dataPtr++;

    // Deserialize the JSON data payload using ArduinoJson
    ArduinoJson::JsonDocument doc;
    ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(doc, dataPtr);
    if (error == ArduinoJson::DeserializationError::Ok) {
        if (!doc["path"].isNull()) {
            dataPath = doc["path"].as<String>();
        }
        if (!doc["data"].isNull()) {
            if (doc["data"].is<ArduinoJson::JsonObject>() || doc["data"].is<ArduinoJson::JsonArray>()) {
                ArduinoJson::serializeJson(doc["data"], dataPayload);
            } else {
                dataPayload = doc["data"].as<String>();
            }
        }
        return true;
    }
    return false;
}
