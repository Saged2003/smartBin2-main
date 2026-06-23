#include "S3AppController.h"
#include "../../include/Config.h" 

extern DisplayManager display; 
extern void sendCapacityToServer(float pLvl, float mLvl);
extern void sendSessionEndToServer(int pts, float wt);

void S3AppController::init() {
    Serial1.begin(115200, SERIAL_8N1, SLAVE_RX, SLAVE_TX);
    cooldownStartTime = 0;
    earnedPoints = 0;
    randomSeed(analogRead(0)); 
    
    currentState = STATE_INIT; 
}

bool S3AppController::isIdle() {
    return (currentState == STATE_IDLE || currentState == STATE_INIT);
}

void S3AppController::receiveBackendCommand(String cmd) {
    if (cmd.startsWith("QR:")) {
        String qrData = cmd.substring(3);
        display.drawQRCode(qrData);
        display.showMessage("Scan to Start");
        currentState = STATE_IDLE; 
    } 
    else if (cmd == "START_BIN" && currentState == STATE_IDLE) {
        Serial1.println("start");
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
                            
                            sendCapacityToServer(pLvl, mLvl); 
                        } 
                        else if (req == "end") {
                            earnedPoints = doc["pts"];
                            float totalWt = doc["wt"]; 
                            drawScoreScreen(earnedPoints);
                            
                            sendSessionEndToServer(earnedPoints, totalWt);
                            
                            cooldownStartTime = millis();
                            currentState = STATE_COOLDOWN;
                        }
                    }
                }
            }
            break;

        case STATE_COOLDOWN:
            if (now - cooldownStartTime >= 3000) {
                currentState = STATE_INIT; 
            }
            break;
    }
}

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

    display.tft.setCursor(10, 15);
    display.tft.print("Last Item: ");
    if (type == 1) display.tft.print("Plastic");
    else if (type == 2) display.tft.print("Metal");
    else display.tft.print("Unknown");

    display.tft.setCursor(10, 40);
    display.tft.print("Plastic Lvl: ");
    display.tft.print(pLvl);
    display.tft.print("%");

    display.tft.setCursor(10, 65);
    display.tft.print("Plastic Wt:  ");
    display.tft.print(pWt);
    display.tft.print(" g");

    display.tft.setCursor(10, 95);
    display.tft.print("Metal Lvl:   ");
    display.tft.print(mLvl);
    display.tft.print("%");

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