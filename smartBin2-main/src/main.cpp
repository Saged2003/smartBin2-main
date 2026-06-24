#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"
#include "S3AppController.h"

DisplayManager display;
S3AppController app;

// --- إعدادات الشبكة والسيرفر (قم بتغييرها بما يناسبك) ---
const char* ssid = "YOUR_WIFI_SSID";         // اكتب اسم شبكة الواي فاي
const char* password = "YOUR_WIFI_PASSWORD"; // اكتب كلمة المرور
const char* mqtt_server = "192.168.1.xxx";   // اكتب الـ IP الخاص بجهاز الكمبيوتر الذي يعمل عليه الباك إند

String BIN_ID = "BIN_001";                   // يجب أن يطابق الـ bin_id المسجل في الداتا بيز
String HARDWARE_TOKEN = "TOKEN_12345";       // التوكن الخاص بالسلة لحمايتها

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool needsNewQR = true;
unsigned long lastQRRequestTime = 0;

// --- استقبال أوامر السيرفر (MQTT) ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    String topicStr = String(topic);

    // إذا استقبلنا QR جديد من السيرفر
    if (topicStr.endsWith("/qr_code")) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            String code = doc["code"].as<String>();
            app.receiveBackendCommand("QR:" + code);
            needsNewQR = false; // استلمنا الكود بنجاح
        }
    }
    // إذا استقبلنا أمر التشغيل (المستخدم قام بعمل مسح للـ QR)
    else if (topicStr.endsWith("/command")) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error && doc["cmd"] == "START_BIN") {
            app.receiveBackendCommand("START_BIN");
        }
    }
}

void setup() {
    Serial.begin(115200);
    display.init();
    app.init();
    
    // الاتصال بالواي فاي
    display.showMessage("Connecting WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    // إعداد الـ MQTT
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqttCallback);
}

void reconnect() {
    if (!mqttClient.connected()) {
        display.showMessage("Connecting MQTT...");
        if (mqttClient.connect(BIN_ID.c_str())) {
            // الاشتراك في قنوات السلة
            mqttClient.subscribe(("smartbin/" + BIN_ID + "/qr_code").c_str());
            mqttClient.subscribe(("smartbin/" + BIN_ID + "/command").c_str());
            
            // مسح الشاشة بعد الاتصال للعودة للحالة الطبيعية
            display.tft.fillScreen(ST77XX_BLACK);
        } else {
            delay(5000);
        }
    }
}

void loop() {
    if (!mqttClient.connected()) {
        reconnect();
    } else {
        mqttClient.loop();
    }
    
    // تشغيل اللوجيك الأساسي للسيستم
    app.run();

    // طلب QR Code جديد إذا كانت السلة فارغة وتحتاج لكود جديد
    if (mqttClient.connected() && app.isIdle() && needsNewQR) {
        unsigned long now = millis();
        if (now - lastQRRequestTime > 5000) { // تأخير 5 ثواني بين الطلبات لتجنب الضغط
            String reqTopic = "smartbin/" + BIN_ID + "/request_qr";
            String payload = "{\"hardware_token\":\"" + HARDWARE_TOKEN + "\"}";
            mqttClient.publish(reqTopic.c_str(), payload.c_str());
            lastQRRequestTime = now;
        }
    }
}