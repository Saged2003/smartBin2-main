#ifndef S3_APP_CONTROLLER_H
#define S3_APP_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"

// الحالات الخاصة بالـ S3
enum S3State { 
    STATE_INIT, 
    STATE_SESSION, 
    STATE_COOLDOWN,
    STATE_IDLE 
};

class S3AppController {
private:
    S3State currentState;
    unsigned long cooldownStartTime;
    int earnedPoints;
    
    // شاشات العرض
    void drawSessionScreen();
    void updateReadings(int type, float pLvl, float mLvl, float pWt, float mWt);
    void drawScoreScreen(int points);

public:
    void init();
    void run();

    // البوابة اللي هتستقبل أوامر الباك إند
    void receiveBackendCommand(String cmd); 
        
    // دالة عشان نعرف منها السيستم فاضي ولا في جلسة
    bool isIdle();
};

#endif