#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// -----------------------------------------
// TFT Display Pins for ESP32-S3 (N16R8)
// -----------------------------------------
#define TFT_CS    10
#define TFT_RST   9
#define TFT_DC    8  
#define TFT_MOSI  11 
#define TFT_SCLK  12 

// أبعاد الشاشة الـ 1.8 بوصة
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

// -----------------------------------------
// QR Code Settings
// -----------------------------------------
#define QR_VERSION 5
#define QR_SCALE   3 // حجم البيكسل عشان يكون واضح على الشاشة


// -----------------------------------------
// UART Pins for Master/Slave Communication
// -----------------------------------------
#define SLAVE_RX  18  // هيتوصل بـ TX بتاع البوردة العادية
#define SLAVE_TX  17  // هيتوصل بـ RX بتاع البوردة العادية


#endif


// // ESP32-CAM UART2
// #define CAM_RX     16 // Connects to CAM TX  (برتقالي)
// #define CAM_TX     17 // Connects to CAM RX  (اصفر)