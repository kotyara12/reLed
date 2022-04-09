/* 
 *  LED control module (flashing / blinking, etc.)
 *  ------------------------------------------------------------------------------------------------
 *  (с) 2020-2021 Разживин Александр | Razzhivin Alexander
 *  https://kotyara12.ru | kotyara12@yandex.ru | tg: @kotyara1971
 *
*/

#ifndef __RE_LED_H__
#define __RE_LED_H__

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "driver/gpio.h"

typedef QueueHandle_t ledQueue_t;

// With this callback function, you can control the LEDs connected via the GPIO expansion boards
typedef void (*ledCustomControl_t) (int8_t ledGPIO, bool ledValue);

// Control commands :: ( manual : https://kotyara12.ru/pubs/iot/led32/ )
typedef enum {
    // Locking and unlocking the LED (for example, suppressing any activity at night)
    // Format: lmEnable "state" (ex: "lmEnable 0" or "lmEnable 1")
    lmEnable = 0,   
                
    // LED off
    // Format: lmOff (ignore parameters)
    // ¯¯¯|______________________________________________________________
    lmOff = 1,      
                
    // LED on
    // Format: lmOn (ignore parameters)
    // ___|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
    lmOn = 2,       

    // One or more flashes with a specified duration and interval between flashes
    // Format: lmFlash "number of flashes" "duration of flashes" "pause between flashes"
    // (for example "lmFlash 3 100 500" or "lmFlash 1 250 250")
    // ___|¯|___|¯|___|¯|________________________________________________
    lmFlash = 3,    
                
    // Continuous flashing: uniform or in series of flashes (for example, three flashes - pause, etc.)
    // Format: lmBlinkOn "number of flashes in a series" "period of flashes in a series" "pause between series"
    // (for example "lmBlinkOn 1 500 500" - uniform or "lmBlinkOn 3 100 5000" - in series)
    // Note: here you cannot set the duration between flashes in a series, it is taken equal to the duration of the flash itself
    // ___|¯|_|¯|_|¯|____________|¯|_|¯|_|¯|____________|¯|_|¯|_|¯|______
    lmBlinkOn = 4,  
                
    // Disable blinking (waiting for next command)
    // Format: lmBlinkOff (ignore parameters)
    // _|¯|_|¯|__________________________________________________________
    lmBlinkOff = 5
} ledMode_t;

typedef struct {
  ledMode_t msgMode;
  uint16_t msgValue1;
  uint16_t msgValue2;
  uint16_t msgValue3;
} ledQueueData_t;

#ifdef __cplusplus
extern "C" {
#endif

// Create LED control task
ledQueue_t ledTaskCreate(int8_t ledGPIO, bool ledHigh, bool blinkPriority, const char* taskName, const uint32_t taskStackSize, ledCustomControl_t customControl);
// Sending a command to the task queue for mode switching
bool ledTaskSend(ledQueue_t ledQueue, const ledMode_t msgMode, const uint16_t msgValue1, const uint16_t msgValue2, const uint16_t msgValue3);
bool ledTaskSendFromISR(ledQueue_t ledQueue, const ledMode_t msgMode, const uint16_t msgValue1, const uint16_t msgValue2, const uint16_t msgValue3);
// Deleting a task
void ledTaskDelete(ledQueue_t ledQueue);

#ifdef __cplusplus
}
#endif

#endif // __RE_LED_H__
