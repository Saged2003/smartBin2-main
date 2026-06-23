#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"
#include "S3AppController.h"

// --- إعدادات الشبكة والـ MQTT ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "192.168.1.XXX"; // حط IP السيرفر بتاعك أو اللاب توب
const int mqtt_port = 1883;

// --- بيانات السلة (لازم تتطابق مع الداتا بيز) ---
const char* bin_id = "BIN-001"; 
const char* hardware_token = "SECRET_TOKEN_123";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
DisplayManager display;
S3AppController app;

unsigned long lastReconnectAttempt = 0;

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println("Message arrived [" + String(topic) + "] " + message);

    String topicStr = String(topic);
    
    // لو استقبلنا كود QR جديد من السيرفر
    if (topicStr.endsWith("/qr_code")) {
        JsonDocument doc;
        deserializeJson(doc, message);
        const char* new_code = doc["code"];
        if (new_code) {
            app.receiveBackendCommand("QR:" + String(new_code));
        }
    }
    // لو استقبلنا أمر التشغيل من الموبايل (بعد ما اليوزر عمل Scan)
    else if (topicStr.endsWith("/command")) {
        JsonDocument doc;
        deserializeJson(doc, message);
        const char* cmd = doc["cmd"];
        if (cmd && String(cmd) == "START_BIN") {
            app.receiveBackendCommand("START_BIN");
        }
    }
}

void reconnectMQTT() {
    if (mqttClient.connect(bin_id)) {
        Serial.println("MQTT connected!");
        // الاشتراك في التوبيكس الخاصة بالسلة دي
        String cmdTopic = String("smartbin/") + bin_id + "/command";
        String qrTopic = String("smartbin/") + bin_id + "/qr_code";
        mqttClient.subscribe(cmdTopic.c_str());
        mqttClient.subscribe(qrTopic.c_str());
        
        // طلب كود QR جديد أول ما يشتغل
        String reqTopic = String("smartbin/") + bin_id + "/request_qr";
        String payload = "{\"hardware_token\": \"" + String(hardware_token) + "\"}";
        mqttClient.publish(reqTopic.c_str(), payload.c_str());
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    display.init();
    // تمرير كائن الـ MQTT للكنترولر عشان يقدر يبعت داتا
    app.init(&mqttClient, bin_id, hardware_token); 
    Serial.println(">>> ESP32-S3 Gateway Ready <<<");
}

void loop() {
    if (!mqttClient.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (reconnectMQTT()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        mqttClient.loop();
    }
    
    // تشغيل اللوجيك الأساسي بدون دوال المحاكاة الوهمية
    app.run();
}