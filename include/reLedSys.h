/* 
 *  Display device status via system LED
 *  -------------------------------------------------------------------------------------------------
 *  (с) 2020-2021 Разживин Александр | Razzhivin Alexander
 *  https://kotyara12.ru | kotyara12@yandex.ru | tg: @kotyara1971
 *
 */

#ifndef __RE_LEDSYS_H__
#define __RE_LEDSYS_H__

#include <esp_types.h>
#include "reLed.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

static const int SYSLED_ERROR               = BIT0;
static const int SYSLED_WARNING             = BIT1;
static const int SYSLED_WIFI_CONNECTED      = BIT2;
static const int SYSLED_WIFI_INET_AVAILABLE = BIT3;
static const int SYSLED_WIFI_ERROR          = BIT4;
static const int SYSLED_MQTT_ERROR          = BIT5;
static const int SYSLED_TELEGRAM_ERROR      = BIT6;
static const int SYSLED_OTHER_PUB_ERROR     = BIT7;
static const int SYSLED_SENSOR_ERROR        = BIT8;
static const int SYSLED_OTA                 = BIT9;

void ledSysInit(const int8_t ledGPIO, const bool ledHigh, const bool ledAutoBlink, ledCustomControl_t customControl);
void ledSysFree();
void ledSysOn(const bool fixed);
void ledSysOff(const bool fixed);
void ledSysSet(const bool newState);
void ledSysSetEnabled(const bool newEnabled);
void ledSysFlashOn(const uint16_t quantity, const uint16_t duration, const uint16_t interval);
void ledSysBlinkOn(const uint16_t quantity, const uint16_t duration, const uint16_t interval);
void ledSysBlinkOff();
void ledSysStateSet(const EventBits_t sysState, const bool forced);
void ledSysStateClear(const EventBits_t sysState, const bool forced);

#ifdef __cplusplus
}
#endif

#endif // __RE_LEDSYS_H__
