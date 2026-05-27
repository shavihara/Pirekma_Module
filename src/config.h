#ifndef CONFIG_H
#define CONFIG_H

// === WiFi Configuration ===
#ifdef WIFI_SSID
#undef WIFI_SSID
#endif
#define WIFI_SSID "LabExpert_1.01" 

#ifdef WIFI_PASSWORD
#undef WIFI_PASSWORD
#endif
#define WIFI_PASSWORD "11111111"

#define WIFI_TIMEOUT_MS 30000       // 30 second WiFi connection timeout

// === Firebase Credentials ===
#ifdef FIREBASE_DATABASE_URL
#undef FIREBASE_DATABASE_URL
#endif
#define FIREBASE_DATABASE_URL "https://tst-09999-default-rtdb.asia-southeast1.firebasedatabase.app"

#ifdef FIREBASE_API_KEY
#undef FIREBASE_API_KEY
#endif
#define FIREBASE_API_KEY "AIzaSyAWOTmomJ9B-aZctfyccEMgn7zzWNZpnIs"

#ifdef FIREBASE_AUTH_EMAIL
#undef FIREBASE_AUTH_EMAIL
#endif
#define FIREBASE_AUTH_EMAIL "device_1_colombo@yourdomain.com"

#ifdef FIREBASE_AUTH_PASSWORD
#undef FIREBASE_AUTH_PASSWORD
#endif
#define FIREBASE_AUTH_PASSWORD "device123"

// === Device & Plant Configuration ===
#ifdef CONFIG_DEVICE_ID
#undef CONFIG_DEVICE_ID
#endif
#define CONFIG_DEVICE_ID "esp32_device_1"

#ifdef CONFIG_PLANT_ID
#undef CONFIG_PLANT_ID
#endif
#define CONFIG_PLANT_ID "plant_colombo"

// === Firebase Paths ===
#define FIREBASE_PATH_TELEMETRY "plants/" CONFIG_PLANT_ID "/devices/" CONFIG_DEVICE_ID "/telemetry"
#define FIREBASE_PATH_INFO "plants/" CONFIG_PLANT_ID "/devices/" CONFIG_DEVICE_ID "/info"
#define FIREBASE_PATH_LIMITS "plants/" CONFIG_PLANT_ID "/devices/" CONFIG_DEVICE_ID "/config/limits"
#define FIREBASE_PATH_DEVICE "plants/" CONFIG_PLANT_ID "/devices/" CONFIG_DEVICE_ID

// === Timing ===
#define TELEMETRY_INTERVAL_MS 10000  // 10 seconds

// === Vercel Webhook ===
#define VERCEL_WEBHOOK_URL "https://your-vercel-webhook-url.vercel.app/api/emergency"

#endif // CONFIG_H
