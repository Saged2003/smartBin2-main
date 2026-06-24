#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"
#include "S3AppController.h"

const char* ssid = "Roqaya";
const char* password = "1234567";
const char* mqtt_server = "192.168.43.111";
const int mqtt_port = 1883;

// --- بيانات السلة ---
const char* BIN_ID = "BIN001"; // الـ ID المتسجل في الداتا بيز
const char* HARDWARE_TOKEN = "MY_SECRET_TOKEN_123"; // التوكن السري للسلة

WiFiClient espClient;
PubSubClient client(espClient);

DisplayManager display;
S3AppController app;

// دالة الاتصال بالواي فاي
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

// دالة استقبال الأوامر من Django (MQTT Callback)
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);

    String topicStr = String(topic);
    
    // إذا Django بعت كود QR جديد
    if (topicStr.endsWith("/qr_code")) {
        JsonDocument doc;
        deserializeJson(doc, message);
        String code = doc["code"];
        app.receiveBackendCommand("QR:" + code);
    }
    // إذا Django بعت أمر البداية (User Scanned QR)
    else if (topicStr.endsWith("/command")) {
        JsonDocument doc;
        deserializeJson(doc, message);
        String cmd = doc["cmd"];
        if (cmd == "START_BIN") {
            app.receiveBackendCommand("START_BIN");
        }
    }
}

// دالة الحفاظ على اتصال الـ MQTT
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // استخدام اسم السلة كـ Client ID
        if (client.connect(BIN_ID)) {
            Serial.println("connected");
            
            // الاشتراك في قنوات السلة 
            String qrTopic = String("smartbin/") + BIN_ID + "/qr_code";
            String cmdTopic = String("smartbin/") + BIN_ID + "/command";
            client.subscribe(qrTopic.c_str());
            client.subscribe(cmdTopic.c_str());
            
            // طلب توكن QR جديد فور الاتصال
            String reqTopic = String("smartbin/") + BIN_ID + "/request_qr";
            String payload = "{\"hardware_token\":\"" + String(HARDWARE_TOKEN) + "\"}";
            client.publish(reqTopic.c_str(), payload.c_str());
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    display.init();
    app.init();
    
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    
    Serial.println(">>> ESP32-S3 IoT Gateway Ready <<<");
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    // تشغيل اللوجيك الأساسي وتمرير البيانات بين البوردة والـ MQTT
    app.run();
}