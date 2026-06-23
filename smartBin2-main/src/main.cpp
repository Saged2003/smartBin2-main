#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"
#include "S3AppController.h"

// إعدادات الشبكة القياسية وسيرفر السيرفر الفعلي
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "192.168.1.100"; // قم بتغييره إلى IP جهاز الكمبيوتر الذي يعمل عليه السيرفر
const char* bin_id = "BIN_01";             // معرّف السلة الثابت المطابق لقاعدة البيانات

WiFiClient espClient;
PubSubClient client(espClient);

DisplayManager display;
S3AppController app;

void setup_wifi() {
    delay(10);
    display.showMessage("Connecting WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    display.showMessage("WiFi Connected!");
    Serial.println("\nWiFi Connected!");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    String currentTopic = String(topic);
    
    // استقبال أمر تفعيل السلة القادم من الموبايل عبر السيرفر
    if (currentTopic == "smartbin/" + String(bin_id) + "/command") {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            String cmd = doc["cmd"];
            if (cmd == "START_BIN") {
                app.receiveBackendCommand("START_BIN");
            }
        }
    }
    // استقبال كود الـ QR المتغير من الباك إند وعرضه فوراً
    else if (currentTopic == "smartbin/" + String(bin_id) + "/qr_code") {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            String qrCode = doc["code"];
            app.receiveBackendCommand("QR:" + qrCode);
        }
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(bin_id)) {
            Serial.println("connected");
            client.subscribe(("smartbin/" + String(bin_id) + "/command").c_str());
            client.subscribe(("smartbin/" + String(bin_id) + "/qr_code").c_str());
            
            // طلب كود QR جديد للمستخدم فور بدء الاتصال بالسيرفر
            client.publish(("smartbin/" + String(bin_id) + "/request_qr").c_str(), "{}");
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
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    app.run();
    
    // معالجة البيانات المستقبلة من البوردة السفلية (الحساسات والمحركات) عبر السيريال وتحويلها فورا للباك إند
    if (Serial1.available()) {
        String incomingData = Serial1.readStringUntil('\n');
        incomingData.trim();
        
        if (incomingData.length() > 0) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, incomingData);
            
            if (!error) {
                String req = doc["req"];
                if (req == "update") {
                    float pLvl = doc["pLvl"];
                    float mLvl = doc["mLvl"];
                    float calculatedCapacity = (pLvl > mLvl) ? pLvl : mLvl; // اعتماد القياس الأعلى لامتلاء الحجرات
                    
                    JsonDocument outDoc;
                    outDoc["bin_id"] = bin_id;
                    outDoc["hardware_token"] = "HW_TOKEN_01";
                    outDoc["capacity"] = calculatedCapacity;
                    
                    String payloadStr;
                    serializeJson(outDoc, payloadStr);
                    client.publish(("smartbin/" + String(bin_id) + "/update").c_str(), payloadStr.c_str());
                } 
                else if (req == "end") {
                    int points = doc["pts"];
                    float totalWeight = (float)doc["pWt"] + (float)doc["mWt"]; // جمع الأوزان المدخلة
                    
                    JsonDocument outDoc;
                    outDoc["bin_id"] = bin_id;
                    outDoc["hardware_token"] = "HW_TOKEN_01";
                    outDoc["points"] = points;
                    outDoc["weight"] = totalWeight;
                    
                    String payloadStr;
                    serializeJson(outDoc, payloadStr);
                    client.publish(("smartbin/" + String(bin_id) + "/session_end").c_str(), payloadStr.c_str());
                    
                    // توليد كود QR جديد تلقائياً بعد مرور 3 ثوانٍ لتجهيز السلة للمستخدم القادم
                    delay(3000);
                    client.publish(("smartbin/" + String(bin_id) + "/request_qr").c_str(), "{}");
                }
            }
        }
    }
}