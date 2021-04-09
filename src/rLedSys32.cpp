#include "rLedSys32.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// -----------------------------------------------------------------------------------------------------------------------
// -------------------------------------------- Importing project parameters ---------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

/* Import global project definitions from "project_config.h"                                                            */
/* WARNING! To make "project_config.h" available here, add "build_flags = -Isrc" to your project configuration file:    */
/*                                                                                                                      */
/* [env]                                                                                                                */
/* build_flags = -Isrc                                                                                                  */
/*                                                                                                                      */
/* and place the file in the src directory of the project                                                               */

#if defined (__has_include)
  /* Import only if file exists in "src" directory */
  #if __has_include("project_config.h") 
    #include "project_config.h"
  #endif
#else
  /* Force import if compiler doesn't support __has_include (ESP8266, etc) */
  #include "project_config.h"
#endif

/* If the parameters were not received, we use the default values. */
#ifndef CONFIG_LEDSYS_NORMAL_QUANTITY
#define CONFIG_LEDSYS_NORMAL_QUANTITY 1
#endif
#ifndef CONFIG_LEDSYS_NORMAL_DURATION
#define CONFIG_LEDSYS_NORMAL_DURATION 75
#endif
#ifndef CONFIG_LEDSYS_NORMAL_INTERVAL
#define CONFIG_LEDSYS_NORMAL_INTERVAL 5000
#endif
#ifndef CONFIG_LEDSYS_ERROR_QUANTITY
#define CONFIG_LEDSYS_ERROR_QUANTITY 1
#endif
#ifndef CONFIG_LEDSYS_ERROR_DURATION
#define CONFIG_LEDSYS_ERROR_DURATION 1000
#endif
#ifndef CONFIG_LEDSYS_ERROR_INTERVAL
#define CONFIG_LEDSYS_ERROR_INTERVAL 1000
#endif
#ifndef CONFIG_LEDSYS_WARNING_QUANTITY
#define CONFIG_LEDSYS_WARNING_QUANTITY 1
#endif
#ifndef CONFIG_LEDSYS_WARNING_DURATION
#define CONFIG_LEDSYS_WARNING_DURATION 1000
#endif
#ifndef CONFIG_LEDSYS_WARNING_INTERVAL
#define CONFIG_LEDSYS_WARNING_INTERVAL 1000
#endif
#ifndef CONFIG_LEDSYS_OTA_QUANTITY
#define CONFIG_LEDSYS_OTA_QUANTITY 1
#endif
#ifndef CONFIG_LEDSYS_OTA_DURATION
#define CONFIG_LEDSYS_OTA_DURATION 100
#endif
#ifndef CONFIG_LEDSYS_OTA_INTERVAL
#define CONFIG_LEDSYS_OTA_INTERVAL 100
#endif
#ifndef CONFIG_LEDSYS_WIFI_ERROR_QUANTITY
#define CONFIG_LEDSYS_WIFI_ERROR_QUANTITY 1
#endif
#ifndef CONFIG_LEDSYS_WIFI_ERROR_DURATION
#define CONFIG_LEDSYS_WIFI_ERROR_DURATION 500
#endif
#ifndef CONFIG_LEDSYS_WIFI_ERROR_INTERVAL
#define CONFIG_LEDSYS_WIFI_ERROR_INTERVAL 500
#endif
#ifndef CONFIG_LEDSYS_WIFI_INIT_QUANTITY
#define CONFIG_LEDSYS_WIFI_INIT_QUANTITY 1
#endif
#ifndef CONFIG_LEDSYS_WIFI_INIT_DURATION
#define CONFIG_LEDSYS_WIFI_INIT_DURATION 250
#endif
#ifndef CONFIG_LEDSYS_WIFI_INIT_INTERVAL
#define CONFIG_LEDSYS_WIFI_INIT_INTERVAL 250
#endif
#ifndef CONFIG_LEDSYS_INET_ERROR_QUANTITY
#define CONFIG_LEDSYS_INET_ERROR_QUANTITY 2
#endif
#ifndef CONFIG_LEDSYS_INET_ERROR_DURATION
#define CONFIG_LEDSYS_INET_ERROR_DURATION 100
#endif
#ifndef CONFIG_LEDSYS_INET_ERROR_INTERVAL
#define CONFIG_LEDSYS_INET_ERROR_INTERVAL 500
#endif
#ifndef CONFIG_LEDSYS_MQTT_ERROR_QUANTITY
#define CONFIG_LEDSYS_MQTT_ERROR_QUANTITY 3
#endif
#ifndef CONFIG_LEDSYS_MQTT_ERROR_DURATION
#define CONFIG_LEDSYS_MQTT_ERROR_DURATION 250
#endif
#ifndef CONFIG_LEDSYS_MQTT_ERROR_INTERVAL
#define CONFIG_LEDSYS_MQTT_ERROR_INTERVAL 1000
#endif
#ifndef CONFIG_LEDSYS_TG_ERROR_QUANTITY
#define CONFIG_LEDSYS_TG_ERROR_QUANTITY 4
#endif
#ifndef CONFIG_LEDSYS_TG_ERROR_DURATION
#define CONFIG_LEDSYS_TG_ERROR_DURATION 250
#endif
#ifndef CONFIG_LEDSYS_TG_ERROR_INTERVAL
#define CONFIG_LEDSYS_TG_ERROR_INTERVAL 1000
#endif
#ifndef CONFIG_LEDSYS_PUB_ERROR_QUANTITY
#define CONFIG_LEDSYS_PUB_ERROR_QUANTITY 5
#endif
#ifndef CONFIG_LEDSYS_PUB_ERROR_DURATION
#define CONFIG_LEDSYS_PUB_ERROR_DURATION 250
#endif
#ifndef CONFIG_LEDSYS_PUB_ERROR_INTERVAL
#define CONFIG_LEDSYS_PUB_ERROR_INTERVAL 1000
#endif

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------ End of import of project parameters ----------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static EventGroupHandle_t _ledSysEventGroup = NULL;

static ledQueue_t _ledSysQueue = NULL;

void ledSysBlinkAuto();

void ledSysInit(const int8_t ledGPIO, const bool ledHigh, const bool ledAutoBlink, ledCustomControl_t customControl)
{
  if (!_ledSysEventGroup) {
    _ledSysEventGroup = xEventGroupCreate();
    xEventGroupClearBits(_ledSysEventGroup, 0x00FFFFFF);
  };

  if (_ledSysQueue == NULL) {
    _ledSysQueue = ledTaskCreate(ledGPIO, ledHigh, "ledSystem", customControl);
  };
  
  if (ledAutoBlink) {
    ledSysBlinkAuto();
  };
}

void ledSysFree()
{
  if (_ledSysQueue) { 
    ledTaskDelete(_ledSysQueue);
  };

  if (_ledSysEventGroup) {
    vEventGroupDelete(_ledSysEventGroup);
  };
}

void ledSysOn(const bool fixed)
{
  if (_ledSysQueue) {
    ledTaskSend(_ledSysQueue, lmOn, (uint16_t)fixed, 0, 0);
  };
}

void ledSysOff(const bool fixed)
{
  if (_ledSysQueue) {
    ledTaskSend(_ledSysQueue, lmOff, (uint16_t)fixed, 0, 0);
  };
}

void ledSysSet(const bool newState)
{
  if (_ledSysQueue) {
    if (newState) {
      ledTaskSend(_ledSysQueue, lmOn, 0, 0, 0);
    }
    else {
      ledTaskSend(_ledSysQueue, lmOff, 0, 0, 0);
    };
  };
}

void ledSysSetEnabled(const bool newEnabled)
{
  if (_ledSysQueue) {
    ledTaskSend(_ledSysQueue, lmEnable, (uint16_t)newEnabled, 0, 0);
  };
}

void ledSysFlashOn(const uint16_t quantity, const uint16_t duration, const uint16_t interval)
{
  if (_ledSysQueue) {
    ledTaskSend(_ledSysQueue, lmFlash, quantity, duration, interval);
  };
}

void ledSysBlinkOn(const uint16_t quantity, const uint16_t duration, const uint16_t interval)
{
  if (_ledSysQueue) {
    ledTaskSend(_ledSysQueue, lmBlinkOn, quantity, duration, interval);
  };
}

void ledSysBlinkOff()
{
  if (_ledSysQueue) {
    ledTaskSend(_ledSysQueue, lmBlinkOff, 0, 0, 0);
  };
}

void ledSysBlinkBits(EventBits_t ledBits)
{
  if (_ledSysEventGroup) {
    if (ledBits & SYSLED_ERROR) {
      ledSysBlinkOn(CONFIG_LEDSYS_ERROR_QUANTITY, CONFIG_LEDSYS_ERROR_DURATION, CONFIG_LEDSYS_ERROR_INTERVAL);
    }
    else if (ledBits & SYSLED_WARNING) {
      ledSysBlinkOn(CONFIG_LEDSYS_WARNING_QUANTITY, CONFIG_LEDSYS_WARNING_DURATION, CONFIG_LEDSYS_WARNING_INTERVAL);
    }
    if (ledBits & SYSLED_OTA) {
      ledSysBlinkOn(CONFIG_LEDSYS_OTA_QUANTITY, CONFIG_LEDSYS_OTA_DURATION, CONFIG_LEDSYS_OTA_INTERVAL);
    }
    else if (ledBits & SYSLED_WIFI_ERROR) {
      ledSysBlinkOn(CONFIG_LEDSYS_WIFI_ERROR_QUANTITY, CONFIG_LEDSYS_WIFI_ERROR_DURATION, CONFIG_LEDSYS_WIFI_ERROR_INTERVAL);
    }
    else if (!(ledBits & SYSLED_WIFI_CONNECTED)) {
      ledSysBlinkOn(CONFIG_LEDSYS_WIFI_INIT_QUANTITY, CONFIG_LEDSYS_WIFI_INIT_DURATION, CONFIG_LEDSYS_WIFI_INIT_INTERVAL);
    }
    else if ((ledBits & SYSLED_WIFI_CONNECTED) && !(ledBits & SYSLED_WIFI_INET_AVAILABLE)) {
      ledSysBlinkOn(CONFIG_LEDSYS_INET_ERROR_QUANTITY, CONFIG_LEDSYS_INET_ERROR_DURATION, CONFIG_LEDSYS_INET_ERROR_INTERVAL);
    }
    else if (ledBits & SYSLED_SENSOR_ERROR) {
      ledSysBlinkOn(CONFIG_LEDSYS_SENSOR_ERROR_QUANTITY, CONFIG_LEDSYS_SENSOR_ERROR_DURATION, CONFIG_LEDSYS_SENSOR_ERROR_INTERVAL);
    }
    else if (ledBits & SYSLED_MQTT_ERROR) {
      ledSysBlinkOn(CONFIG_LEDSYS_MQTT_ERROR_QUANTITY, CONFIG_LEDSYS_MQTT_ERROR_DURATION, CONFIG_LEDSYS_MQTT_ERROR_INTERVAL);
    }
    else if (ledBits & SYSLED_TELEGRAM_ERROR) {
      ledSysBlinkOn(CONFIG_LEDSYS_TG_ERROR_QUANTITY, CONFIG_LEDSYS_TG_ERROR_DURATION, CONFIG_LEDSYS_TG_ERROR_INTERVAL);
    }
    else if (ledBits & SYSLED_OTHER_PUB_ERROR) {
      ledSysBlinkOn(CONFIG_LEDSYS_PUB_ERROR_QUANTITY, CONFIG_LEDSYS_PUB_ERROR_DURATION, CONFIG_LEDSYS_PUB_ERROR_INTERVAL);
    }
    else {
      ledSysBlinkOn(CONFIG_LEDSYS_NORMAL_QUANTITY, CONFIG_LEDSYS_NORMAL_DURATION, CONFIG_LEDSYS_NORMAL_INTERVAL);
    };
  };
}

void ledSysBlinkAuto()
{
  ledSysBlinkBits(xEventGroupGetBits(_ledSysEventGroup));
}

void ledSysStateSet(const EventBits_t sysState, const bool forced) 
{
  if ((_ledSysEventGroup) && (_ledSysQueue)) {
    EventBits_t oldState = xEventGroupGetBits(_ledSysEventGroup);
    xEventGroupSetBits(_ledSysEventGroup, sysState);    
    EventBits_t newState = xEventGroupGetBits(_ledSysEventGroup);
    if (forced || (oldState != newState)) {
      ledSysBlinkBits(newState);
    };
  };
}

void ledSysStateClear(const EventBits_t sysState, const bool forced) 
{
  if ((_ledSysEventGroup) && (_ledSysQueue)) {
    EventBits_t oldState = xEventGroupGetBits(_ledSysEventGroup);
    xEventGroupClearBits(_ledSysEventGroup, sysState);    
    EventBits_t newState = xEventGroupGetBits(_ledSysEventGroup);
    if (forced || (oldState != newState)) {
      ledSysBlinkBits(newState);
    };
  };
}

