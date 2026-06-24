#include "S3AppController.h"
#include "../../include/Config.h" 
#include <PubSubClient.h> // استدعاء مكتبة MQTT

extern DisplayManager display; 

// استيراد متغيرات الـ MQTT من ملف main.cpp للاتصال بالسيرفر
extern PubSubClient mqttClient;
extern String BIN_ID;
extern String HARDWARE_TOKEN;
extern bool needsNewQR;

void S3AppController::init() {
    Serial1.begin(115200, SERIAL_8N1, SLAVE_RX, SLAVE_TX);
    cooldownStartTime = 0;
    earnedPoints = 0;
    lastTotalWeight = 0.0;
    randomSeed(analogRead(0)); 
    
    currentState = STATE_INIT; 
}

bool S3AppController::isIdle() {
    return (currentState == STATE_IDLE || currentState == STATE_INIT);
}

// ==========================================
// بوابة استقبال أوامر الباك إند
// ==========================================
void S3AppController::receiveBackendCommand(String cmd) {
    if (cmd.startsWith("QR:")) {
        String qrData = cmd.substring(3);
        display.drawQRCode(qrData);
        display.showMessage("Scan to Start");
        currentState = STATE_IDLE; 
    } 
    else if (cmd == "START_BIN" && currentState == STATE_IDLE) {
        Serial1.println("start"); // إرسال أمر البدء للمتحكم الآخر
        drawSessionScreen();
        currentState = STATE_SESSION;
    }
}

void S3AppController::run() {
    unsigned long now = millis();

    switch (currentState) {
        
        case STATE_INIT:
            display.tft.fillScreen(ST77XX_BLACK);
            display.tft.setTextColor(ST77XX_WHITE);
            display.tft.setTextSize(1);
            display.tft.setCursor(10, 60);
            display.tft.print("Waiting for Server...");
            currentState = STATE_IDLE;
            break;

        case STATE_IDLE:
            // هنا السيستم قاعد مستني الباك إند يبعتله حاجة عن طريق دالة receiveBackendCommand
            break;

        case STATE_SESSION:
            if (Serial1.available()) {
                String incomingData = Serial1.readStringUntil('\n');
                incomingData.trim();

                if (incomingData.length() > 0) {
                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, incomingData);

                    if (!error) {
                        String req = doc["req"];
                        
                        if (req == "update") {
                            int type = doc["type"];
                            float pLvl = doc["pLvl"];
                            float mLvl = doc["mLvl"];
                            float pWt = doc["pWt"];
                            float mWt = doc["mWt"];
                            updateReadings(type, pLvl, mLvl, pWt, mWt);
                            
                            // تحديث الوزن الكلي
                            lastTotalWeight = pWt + mWt;

                            // إرسال التحديث للسيرفر (MQTT)
                            String updateTopic = "smartbin/" + BIN_ID + "/update";
                            JsonDocument outDoc;
                            outDoc["hardware_token"] = HARDWARE_TOKEN;
                            outDoc["capacity"] = (pLvl > mLvl) ? pLvl : mLvl; 
                            String outPayload;
                            serializeJson(outDoc, outPayload);
                            if(mqttClient.connected()) {
                                mqttClient.publish(updateTopic.c_str(), outPayload.c_str());
                            }
                        } 
                        else if (req == "end") {
                            earnedPoints = doc["pts"];
                            drawScoreScreen(earnedPoints);
                            
                            // إرسال إنتهاء الجلسة للسيرفر (MQTT)
                            String endTopic = "smartbin/" + BIN_ID + "/session_end";
                            JsonDocument endDoc;
                            endDoc["hardware_token"] = HARDWARE_TOKEN;
                            endDoc["points"] = earnedPoints;
                            endDoc["weight"] = lastTotalWeight;
                            String endPayload;
                            serializeJson(endDoc, endPayload);
                            if(mqttClient.connected()) {
                                mqttClient.publish(endTopic.c_str(), endPayload.c_str());
                            }
                            
                            cooldownStartTime = millis();
                            currentState = STATE_COOLDOWN;
                            needsNewQR = true; // السماح بطلب QR جديد للجلسة القادمة
                        }
                    }
                }
            }
            break;

        case STATE_COOLDOWN:
            // عرض النتيجة لمدة 3 ثواني، ثم العودة لانتظار سيرفر الباك إند
            if (now - cooldownStartTime >= 3000) {
                currentState = STATE_INIT; 
            }
            break;
    }
}

// ==========================================
// دوال الرسم على الشاشة
// ==========================================

void S3AppController::drawSessionScreen() {
    display.tft.fillScreen(ST77XX_BLACK);
    display.tft.setTextColor(ST77XX_WHITE);
    display.tft.setTextSize(1);
    
    display.tft.setCursor(10, 20);
    display.tft.print("Session Active");
    
    display.tft.setCursor(10, 60);
    display.tft.print("Waiting for waste...");
}

void S3AppController::updateReadings(int type, float pLvl, float mLvl, float pWt, float mWt) {
    display.tft.fillScreen(ST77XX_BLACK);
    display.tft.setTextColor(ST77XX_WHITE);
    display.tft.setTextSize(1);

    // السطر الأول: نوع المخلفات الحالية
    display.tft.setCursor(10, 15);
    display.tft.print("Last Item: ");
    if (type == 1) display.tft.print("Plastic");
    else if (type == 2) display.tft.print("Metal");
    else display.tft.print("Unknown");

    // السطر الثاني: مستوى البلاستيك
    display.tft.setCursor(10, 40);
    display.tft.print("Plastic Lvl: ");
    display.tft.print(pLvl);
    display.tft.print("%");

    // السطر الثالث: وزن البلاستيك الكلي
    display.tft.setCursor(10, 65);
    display.tft.print("Plastic Wt:  ");
    display.tft.print(pWt);
    display.tft.print(" g");

    // السطر الرابع: مستوى المعدن
    display.tft.setCursor(10, 95);
    display.tft.print("Metal Lvl:   ");
    display.tft.print(mLvl);
    display.tft.print("%");

    // السطر الخامس: وزن المعدن الكلي
    display.tft.setCursor(10, 120);
    display.tft.print("Metal Wt:    ");
    display.tft.print(mWt);
    display.tft.print(" g");
}

void S3AppController::drawScoreScreen(int points) {
    display.tft.fillScreen(ST77XX_GREEN);
    display.tft.setTextColor(ST77XX_BLACK);
    display.tft.setTextSize(2);

    display.tft.setCursor(20, 40);
    display.tft.print("Session End!");

    display.tft.setTextSize(3);
    display.tft.setCursor(40, 80);
    display.tft.print("+");
    display.tft.print(points);
    
    display.tft.setTextSize(1);
    display.tft.setCursor(20, 130);
    display.tft.print("Next session in 3s...");
}