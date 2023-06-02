#include "reLed.h"
#include "rLog.h"
#include "esp_task_wdt.h"

#if CONFIG_RLOG_PROJECT_LEVEL > RLOG_LEVEL_NONE
static const char * logTAG = "RLED";
#endif // CONFIG_RLOG_PROJECT_LEVEL

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
  #if __has_include("def_consts.h") 
    #include "def_consts.h"
  #endif
#else
  /* Force import if compiler doesn't support __has_include (ESP8266, etc) */
  #include "project_config.h"
  #include "def_consts.h"
#endif

/* If the parameters were not received, we use the default values. */
#ifndef CONFIG_LED_QUEUE_SIZE
#define CONFIG_LED_QUEUE_SIZE 8
#endif
#ifndef CONFIG_LED_TASK_STACK_SIZE
#define CONFIG_LED_TASK_STACK_SIZE 2048
#endif
#ifndef CONFIG_TASK_PRIORITY_LED
#define CONFIG_TASK_PRIORITY_LED 5
#endif
#ifndef CONFIG_TASK_CORE_LED
#define CONFIG_TASK_CORE_LED 1
#endif

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------ End of import of project parameters ----------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

class espLed {
  private:
    int8_t _ledGPIO = -1;
    bool _ledHigh = true;
    ledCustomControl_t _customControl = NULL;
    bool _ledEnabled = true;
    bool _ledInit = false;
    bool _ledState = false;
    TickType_t _ledWait = portMAX_DELAY;
    // on mode
    bool _ledOn = false;
    uint16_t _onCount = 0;
    // flash mode
    bool _ledFlash = false;
    uint16_t _flashDuration = 0;
    uint16_t _flashQuantity = 0;
    uint16_t _flashInterval = 0;
    uint16_t _flashCount = 0;
    // blink mode
    bool _ledBlink = false;
    uint16_t _blinkDuration = 0;
    uint16_t _blinkQuantity = 0;
    uint16_t _blinkInterval = 0;
    uint16_t _blinkCount = 0;
    bool _ledBlinkPriority = false;

    void ledSetLevel(bool newLevel);
    void ledSetState(bool newState);
  public:
    espLed(int8_t ledGPIO, bool ledHigh, bool blinkPriority, ledCustomControl_t customControl = NULL);

    void initGPIO();

    void ledEnabled(bool newEnabled);

    TickType_t getTimeout();

    void ledOn(const bool fixed);
    void ledOff(const bool fixed);

    void flashOn(uint16_t flashQuantity, uint16_t flashDuration, uint16_t flashInterval);
    void flashOff();
    void blinkOn(uint16_t blinkQuantity, uint16_t blinkDuration, uint16_t blinkInterval);
    void blinkOff();

    void processTimeout();
};

espLed::espLed(int8_t ledGPIO, bool ledHigh, bool blinkPriority, ledCustomControl_t customControl)
{
  _ledGPIO = ledGPIO;
  _ledHigh = ledHigh;
  _ledBlinkPriority = blinkPriority;
  _customControl = customControl;
  _ledInit = false;
  _ledOn = false;
  _ledWait = portMAX_DELAY;
  _onCount = 0;
  _ledFlash = false;
  _flashDuration = 0;
  _flashQuantity = 0;
  _flashInterval = 0;
  _flashCount = 0;
  _ledBlink = false;
  _blinkDuration = 0;
  _blinkQuantity = 0;
  _blinkInterval = 0;
  _blinkCount = 0;
};

// GPIO initialization
void espLed::initGPIO() 
{
  _ledInit = true;

  if (_customControl == NULL) {
    gpio_reset_pin(static_cast<gpio_num_t>(_ledGPIO));
    gpio_set_direction(static_cast<gpio_num_t>(_ledGPIO), GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(static_cast<gpio_num_t>(_ledGPIO), GPIO_FLOATING);
  };
  ledSetLevel(_ledState);
};

// Physical control of GPIOs without changing internal variables
void espLed::ledSetLevel(bool newLevel)
{
  if (!_ledInit) {
    initGPIO();
  };

  if (_ledGPIO > -1) {
    if (_customControl == NULL) {
      if (_ledHigh) {
        gpio_set_level(static_cast<gpio_num_t>(_ledGPIO), newLevel);
      } else {
        gpio_set_level(static_cast<gpio_num_t>(_ledGPIO), !newLevel);
      };
    }
    else {
      _customControl(_ledGPIO, (newLevel == _ledHigh));
    };
  };
};

// GPIO control with remembering the last status
void espLed::ledSetState(bool newState)
{
  if (_ledState != newState) {
    _ledState = newState;
    if (_ledEnabled) {
      ledSetLevel(_ledState);
    };
  };
}

// Getting the queue timeout
TickType_t espLed::getTimeout()
{
  return _ledWait;
}

// Unlock or lock the LED
// PS: There is a non-critical bug here: during blinking, switching the Enabled mode will reset the current period
// (either the flash will drag on, or a pause - how lucky)
void espLed::ledEnabled(bool newEnabled)
{
  if (_ledEnabled != newEnabled) {
    _ledEnabled = newEnabled;
    if (_ledEnabled) {
      rlog_v(pcTaskGetTaskName(NULL), "LED enabled");
      // Turn on the LED if it is enabled and it should be on by _ledState
      ledSetLevel(_ledState);
    }
    else {
      rlog_v(pcTaskGetTaskName(NULL), "LED disabled");
      // Turn off the LED without changing the status (the status will change as before, but the LED will remain off)
      ledSetLevel(false);
    };
  };
}

// Turning on the LED
void espLed::ledOn(const bool fixed)
{
  if (fixed) _onCount++;
  rlog_v(pcTaskGetTaskName(NULL), "LED on : %d", _onCount);
  _ledOn = true;
  if (!_ledBlinkPriority) {
    _ledFlash = false;
    _ledBlink = false;
  };
  _ledWait = portMAX_DELAY;
  ledSetState(true);
}

// Turn off LED or restore last blinking mode
void espLed::ledOff(const bool fixed) 
{
  if (fixed && (_onCount > 0)) _onCount--;
  if (_onCount > 0) {
    rlog_v(pcTaskGetTaskName(NULL), "LED off blocked: %d", _onCount);
  } else {
    rlog_v(pcTaskGetTaskName(NULL), "LED off: %d", _onCount);
    _ledOn = false;
    if (_ledBlinkPriority) {
      if (_ledFlash && (_flashInterval > 0)) {
        _ledWait = pdMS_TO_TICKS(_flashInterval); 
      } else if (_ledBlink && (_blinkInterval > 0)) {
        _ledWait = pdMS_TO_TICKS(_blinkInterval); 
        _blinkCount = 0;
      } else {
        _ledWait = portMAX_DELAY;
      };
    } else {
      _ledFlash = false;
      _ledBlink = false;
      _ledWait = portMAX_DELAY;
    };
    ledSetState(false);
  };
}

// Activation of flashing single series
void espLed::flashOn(uint16_t flashQuantity, uint16_t flashDuration, uint16_t flashInterval)
{
  rlog_v(pcTaskGetTaskName(NULL), "LED flash on : %d, %d, %d", flashQuantity, flashDuration, flashInterval);

  // Set parameters and reset the flash counter
  _ledFlash = true;
  _flashDuration = flashDuration;
  _flashQuantity = flashQuantity;
  _flashInterval = flashInterval;
  _flashCount = 0;

  if (!_ledBlinkPriority) {
    _ledOn = false;
    _ledBlink = false;
  };

  // Turn on the LED if the continuous light is not blocked by the counter
  if (_onCount == 0) {
    if (_ledOn) _ledOn = false;
    _ledWait = 0; 
    ledSetState(false);
  };
}

// Turn off flashing single series
void espLed::flashOff() 
{
  if (_ledFlash) {
    rlog_v(pcTaskGetTaskName(NULL), "LED flash off");
    _ledFlash = false;
    
    // Turn on the LED if the continuous light is not blocked by the counter
    if (_onCount == 0) {
      if (_ledOn) _ledOn = false;
      if (_ledBlinkPriority && _ledBlink) {
        _ledWait = pdMS_TO_TICKS(_blinkInterval); 
        _blinkCount = 0;
      } else {
        _ledBlink = false;
        _ledWait = portMAX_DELAY;  
      };
      ledSetState(false);
    };
  };
};

// Activation of the flashing mode in continuous series
void espLed::blinkOn(uint16_t blinkQuantity, uint16_t blinkDuration, uint16_t blinkInterval)
{
  rlog_v(pcTaskGetTaskName(NULL), "LED blink on : %d, %d, %d", blinkQuantity, blinkDuration, blinkInterval);

  // Set parameters and reset the flash counter
  _ledBlink = true;
  _blinkDuration = blinkDuration;
  _blinkQuantity = blinkQuantity;
  _blinkInterval = blinkInterval;
  _blinkCount = 0;

  // Turn on the LED if the continuous light is not blocked by the counter
  if ((_onCount == 0) && !_ledFlash) {
    if (_ledOn) _ledOn = false;
    _ledWait = 0; 
    ledSetState(false);
  };
}

// Turn off blinking mode
void espLed::blinkOff() 
{
  if (_ledBlink) {
    rlog_v(pcTaskGetTaskName(NULL), "LED blink off");
    _ledBlink = false;
    _ledFlash = false;
    
    // Turn on the LED if the continuous light is not blocked by the counter
    if (_onCount == 0) {
      if (_ledOn) _ledOn = false;
      ledSetState(false);
    };
    _ledWait = portMAX_DELAY;
  };
};

/************************************************************************************************************************/

// Timeout handling
void espLed::processTimeout()
{
  if (_ledFlash) {
    // Flashing single series
    if (_ledState) {
      if (++_flashCount >= _flashQuantity) {
        _flashCount = 0;
        flashOff();
      } else {
        _ledWait = pdMS_TO_TICKS(_flashInterval);
        ledSetState(false);
      };
    } else {
      _ledWait = pdMS_TO_TICKS(_flashDuration);
      ledSetState(true);
    };
  } else if (_ledBlink) {
    // Flashing mode in continuous series
    if (_ledState) {
      if (++_blinkCount >= _blinkQuantity) {
        _blinkCount = 0;
        _ledWait = pdMS_TO_TICKS(_blinkInterval);
      } else {
        _ledWait = pdMS_TO_TICKS(_blinkDuration);
      };
      ledSetState(false);
    } else {
      _ledWait = pdMS_TO_TICKS(_blinkDuration);
      ledSetState(true);
    };
  };
}

/************************************************************************************************************************/

typedef struct {
  espLed* ledInstance;
  QueueHandle_t ledQueue;
} ledHandles_t;

// Task function
void ledTaskExec(void *pvParameters)
{
  ledHandles_t* _hHandles = (ledHandles_t*)pvParameters;
  ledQueueData_t msgQueue;

  if ((_hHandles != NULL) && (_hHandles->ledInstance != NULL)) {
    // Initialize GPIO and turn off the LED
    _hHandles->ledInstance->initGPIO();

    // The task cycle is executed while the control queue exists
    while ((_hHandles != NULL) && (_hHandles->ledQueue != NULL)) {
      // We are waiting for the incoming command to switch the mode
      if (xQueueReceive(_hHandles->ledQueue, &msgQueue, _hHandles->ledInstance->getTimeout()) == pdPASS) {
        rlog_v(pcTaskGetTaskName(NULL), "New command recieved: %d, %d, %d, %d", msgQueue.msgMode, msgQueue.msgValue1, msgQueue.msgValue2, msgQueue.msgValue3);

        switch (msgQueue.msgMode) {
          case lmEnable:  
            // Allow or prohibit the operation of the LED
            _hHandles->ledInstance->ledEnabled((bool)msgQueue.msgValue1);
            break;

          case lmOn: 
            // We turn on the LED constantly
            _hHandles->ledInstance->ledOn((bool)msgQueue.msgValue1);
            break;

          case lmOff:
            // Turn off the LED constantly
            _hHandles->ledInstance->ledOff((bool)msgQueue.msgValue1);
            break;
            
          case lmFlash:
            // Single flash or series of flashes
            _hHandles->ledInstance->flashOn(msgQueue.msgValue1, msgQueue.msgValue2, msgQueue.msgValue3);
            break;

          case lmBlinkOn:
            // Enable continuous flashing
            _hHandles->ledInstance->blinkOn(msgQueue.msgValue1, msgQueue.msgValue2, msgQueue.msgValue3);
            break;

          case lmBlinkOff:
            // Disable continuous flashing
            _hHandles->ledInstance->blinkOff();
            break;
        };
      };

      // If you get here, it means there was a timeout exit and it's time to switch the LED
      _hHandles->ledInstance->processTimeout();
      vTaskDelay(1);
    };

    // If for some reason the queue has not been deleted, delete it
    if (_hHandles->ledQueue != NULL) {
      rlog_v(pcTaskGetTaskName(NULL), "Delete LED control queue");
      vQueueDelete(_hHandles->ledQueue);
    };

    // Removing the instance of the control object
    if (_hHandles->ledInstance != NULL ) {
      rlog_v(pcTaskGetTaskName(NULL), "Delete LED control instance");
      delete _hHandles->ledInstance;
    };

    delete _hHandles;
  };

  // Delete the task itself
  rlog_v(pcTaskGetTaskName(NULL), "Delete LED control task");
  vTaskDelete(NULL);
}

// Create LED control task
ledQueue_t ledTaskCreate(int8_t ledGPIO, bool ledHigh, bool blinkPriority, const char* taskName, const uint32_t taskStackSize, ledCustomControl_t customControl)
{
  // Create a message queue to control the LED
  rlog_v(logTAG, "Creating message queue to control LED on GPIO %d", ledGPIO);
  ledQueue_t ledQueue = xQueueCreate(CONFIG_LED_QUEUE_SIZE, sizeof(ledQueueData_t));
  if (ledQueue == NULL) {
    rlog_e(logTAG, "Error creating message queue to control LED on GPIO %d", ledGPIO);
    return NULL;
  };

  // Create a record for storing task parameters
  ledHandles_t *handles = new ledHandles_t;
  handles->ledQueue = ledQueue;

  // Create an LED control object
  rlog_v(logTAG, "Creating an LED control instance on GPIO %d", ledGPIO);
  handles->ledInstance = new espLed(ledGPIO, ledHigh, blinkPriority, customControl);
  if (handles->ledInstance == NULL) {
    delete handles;
    vQueueDelete(ledQueue);
    rlog_e(logTAG, "Error creating LED control object on GPIO %d", ledGPIO);
    return NULL;
  };

  // Create a task to control the LED
  rlog_i(logTAG, "Creating task [ %s ] to control LED on GPIO %d, stack size in bytes: %d", taskName, ledGPIO, taskStackSize);
  TaskHandle_t ledTask;
  xTaskCreatePinnedToCore(ledTaskExec, taskName, taskStackSize, (void*)handles, CONFIG_TASK_PRIORITY_LED, &ledTask, CONFIG_TASK_CORE_LED);
  if (ledTask == NULL) {
    delete handles;
    vQueueDelete(ledQueue);
    rlog_e(logTAG, "Error creating task [ %s ] to control LED on GPIO %d", taskName, ledGPIO);
    return NULL;
  };

  return ledQueue;
}

// Sending a command to the task queue for mode switching
bool ledTaskSend(ledQueue_t ledQueue, const ledMode_t msgMode, const uint16_t msgValue1, const uint16_t msgValue2, const uint16_t msgValue3)
{
  if (ledQueue != NULL) {
    // Create a message
    ledQueueData_t msgQueue;
    msgQueue.msgMode = msgMode;
    msgQueue.msgValue1 = msgValue1;
    msgQueue.msgValue2 = msgValue2;
    msgQueue.msgValue3 = msgValue3;

    // Putting the created message into the queue
    if (xQueueSend(ledQueue, &msgQueue, CONFIG_LED_QUEUE_WAIT) == pdPASS) {
      rlog_v(pcTaskGetTaskName(NULL), "New command send for LED: %d, %d, %d, %d", msgQueue.msgMode, msgQueue.msgValue1, msgQueue.msgValue2, msgQueue.msgValue3);
      return true;
    };
  };
  return false;
}

bool ledTaskSendFromISR(ledQueue_t ledQueue, const ledMode_t msgMode, const uint16_t msgValue1, const uint16_t msgValue2, const uint16_t msgValue3)
{
  if (ledQueue != NULL) {
    // Create a message
    ledQueueData_t msgQueue;
    msgQueue.msgMode = msgMode;
    msgQueue.msgValue1 = msgValue1;
    msgQueue.msgValue2 = msgValue2;
    msgQueue.msgValue3 = msgValue3;

    // We have not woken a task at the start of the ISR.
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    // Putting the created message into the queue
    if (xQueueSendFromISR(ledQueue, &msgQueue, &xHigherPriorityTaskWoken) == pdPASS) {
      if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
      return true;
    };
  };
  return false;
}

// Deleting a task
void ledTaskDelete(ledQueue_t ledQueue)
{
  // If the LED queue exists, then delete it
  // The task will self-destruct in ledTaskExec along with handles
  if (ledQueue != NULL) {
    rlog_v(logTAG, "Delete LED control queue");
    vQueueDelete(ledQueue);
    ledQueue = NULL;
  };
}
