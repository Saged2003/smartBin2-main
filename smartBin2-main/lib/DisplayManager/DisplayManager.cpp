#include "DisplayManager.h"

// تمرير البنات للكلاس بتاع الشاشة
DisplayManager::DisplayManager() : tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST) {}

void DisplayManager::init() {
    // تهيئة الشاشة (النوع BlackTab هو الأشهر في الشاشات الـ 1.8)
    tft.initR(INITR_BLACKTAB); 
    
    // ضبط اتجاه الشاشة (0 أو 1 أو 2 أو 3)
    tft.setRotation(0); 
    
    // تلوين الشاشة بالأسود كبداية
    tft.fillScreen(ST77XX_BLACK);
}

void DisplayManager::drawQRCode(String text) {
    // تلوين الخلفية بالأبيض لأن الـ QR لازم يكون خلفيته بيضاء
    tft.fillScreen(ST77XX_WHITE);

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];
    
    // تحويل النص إلى بيانات QR
    qrcode_initText(&qrcode, qrcodeData, QR_VERSION, 0, text.c_str());

    // حساب حجم الـ QR عشان نوسطنه في الشاشة
    int qrSize = qrcode.size * QR_SCALE;
    int startX = (TFT_WIDTH - qrSize) / 2;
    int startY = (TFT_HEIGHT - qrSize) / 2;

    // رسم مربعات الـ QR Code
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                // رسم بيكسل أسود
                tft.fillRect(startX + x * QR_SCALE, startY + y * QR_SCALE, QR_SCALE, QR_SCALE, ST77XX_BLACK);
            }
        }
    }
}

void DisplayManager::showMessage(String msg) {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(10, TFT_HEIGHT - 20); // وضع النص في أسفل الشاشة
    tft.print(msg);
}