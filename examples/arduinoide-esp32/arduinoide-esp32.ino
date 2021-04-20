/*
   Demo Project for LED Control Module
   ----------------------------------------------------------------------------------
   (с) 2020 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#include <Arduino.h>
#include "reLed.h"
#include "reLedSys.h"
#include "rLog.h"
#include "project_config.h"

void setup() {
   // put your setup code here, to run once:
   Serial.begin(115200);

   // LED initialization
   // On GPIO 17, let there be a "system" LED, which supposedly displays a connection to WiFi
   ledSysInit(17, true, true, NULL);
   ledQueue_t ledBlue = ledTaskCreate(16, true, "ledBlue", NULL);
   ledQueue_t ledGreen = ledTaskCreate(19, true, "ledGreen", NULL);
   ledQueue_t ledYellow = ledTaskCreate(18, true, "ledYellow", NULL);

   // Setting the blinking parameters for the remaining LEDs
   ledTaskSend(ledBlue, lmBlinkOn, 1, 500, 500);
   ledTaskSend(ledGreen, lmBlinkOn, 3, 100, 2000);
   ledTaskSend(ledYellow, lmBlinkOn, 1, 250, 1000);

   // Wait 10 seconds
   vTaskDelay(10000 / portTICK_RATE_MS);

   // "Connected" to WiFi
   ledSysStateSet(SYSLED_WIFI_CONNECTED);
   vTaskDelay(5000 / portTICK_RATE_MS);
   ledSysStateSet(SYSLED_WIFI_INET_AVAILABLE);

   // Wait 20 seconds
   vTaskDelay(20000 / portTICK_RATE_MS);

   // One-time flashes, after which we return to the previous mode
   ledSysFlashOn(5, 100, 250);
}

void loop() {
  // put your main code here, to run repeatedly:
  // nothing
}

