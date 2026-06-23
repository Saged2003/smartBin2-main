#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "qrcode.h"
#include "Config.h"

class DisplayManager {
public: // <-- خلينا الشاشة Public عشان الـ main يقدر يتحكم فيها
    Adafruit_ST7735 tft;

    DisplayManager();
    void init();
    void drawQRCode(String text);
    void showMessage(String msg);
};

#endif