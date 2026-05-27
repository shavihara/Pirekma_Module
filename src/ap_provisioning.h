#ifndef AP_PROVISIONING_H
#define AP_PROVISIONING_H

#include <Arduino.h>

struct Credentials {
    char ssid[32];
    char password[64];
};

extern bool apProvisioningActive;

void initAPProvisioning();
void handleAPClient();
void clearWiFiCredentials();
bool hasWiFiCredentials();
void getWiFiCredentials(String& ssid, String& password);

#endif
