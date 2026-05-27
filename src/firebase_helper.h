#ifndef FIREBASE_HELPER_H
#define FIREBASE_HELPER_H

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <FirebaseClient.h>
#include <ArduinoJson.h>

using AsyncClient = AsyncClientClass;
extern AsyncClient aClient;

extern UserAuth user_auth;
extern FirebaseApp app;
extern RealtimeDatabase Database;
extern AsyncResult databaseResult;
extern AsyncResult limitsResult;

// Public Functions
void initFirebase();
void fetchLimits();
void sendTelemetry();
void processFirebaseData();

#endif // FIREBASE_HELPER_H
