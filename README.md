# Library for easy control of one or more LEDs.

Supported modes on / off, flash burst, blinking (uniform or burst). There can be one or several LEDs, each LED has its own FreeRTOS task, or use a "system" LED (a task for the system LED will be created automatically).

It is supported to turn on the LED both by setting the high level and low (depending on the LED connection scheme). In addition, work not only with the GPIO of the microcontroller is supported, but also the control of LEDs through the I2C expansion board of the PCF8574 type using a custom callback function.

It is possible to forcibly suppress LED flashes for a while (for example, you can turn off the LEDs on devices at night, so as not to interfere with sleep).

The library extension for the "system" LED reLedSys allows the device status to be indicated using a single LED (as in car alarms and printers). In addition, the functions of the system LED can be used in a wide variety of places in the code without having to worry about which pin and how the main LED is connected.

Description: https://kotyara12.ru/pubs/iot/led32/


## USING

To create a task for controlling an LED, use the ledTaskCreate function:

ledQueue_t ledTaskCreate(const int8_t ledGPIO, const bool ledHigh, const char * taskName, ledCustomControl_t customControl);

Where:
ledGPIO - pin number to which the LED is connected.
ledHigh - true = enabled by setting to GPIO high, false = by setting to GPIO low.
taskName is the name of the task, for example "ledRed" or "ledAlarm".
customControl is a callback function to control addressable LEDs or expansion cards (for example PCF8574). Specify NULL if not required.

The return value is a pointer to the instance of the queue associated with the running task (not the task!).

After that, you can send commands to switch the operating mode to the created queue using the following function:

bool ledTaskSend(ledQueue_t ledQueue, ledMode_t msgMode, uint16_t msgValue1, uint16_t msgValue2, uint16_t msgValue3);

Where:
ledQueue is a pointer to the queue created in ledTaskCreate (...).
msgMode - set operating mode (or command), the list of possible commands see below
msgValue1, msgValue2, msgValue3 - passed values. Some commands do not require a value, specify 0 or any other number.


## OPERATING MODES / CONTROL COMMANDS

- lmEnable
Locking and unlocking the LED (for example, suppressing any activity at night)
Format: lmEnable "state" (ex: "lmEnable 0" or "lmEnable 1")
   
- lmOff			
LED off
Format: lmOff (ignore parameters)

- lmOn			
LED on
Format: lmOn (ignore parameters)

- lmFlash
One or more flashes with a specified duration and interval between flashes
Format: lmFlash "number of flashes" "duration of flashes" "pause between flashes"
(for example "lmFlash 3 100 500" or "lmFlash 1 250 250")

- lmBlinkOn
Continuous flashing: uniform or in series of flashes (for example, three flashes - pause, etc.)
Format: lmBlinkOn "number of flashes in a series" "period of flashes in a series" "pause between series"
(for example "lmBlinkOn 1 500 500" - uniform or "lmBlinkOn 3 100 5000" - in series)
Note: here you cannot set the duration between flashes in a series, it is taken equal to the duration of the flash itself

- lmBlinkOff  
Disable blinking (waiting for next command)
Format: lmBlinkOff (ignore parameters)

!!! The blinkSet mode is autosave !!!
If the lmBlinkOn command was received, and after it any other mode (lmOn / lmFlash) was activated, then after its completion (the lmOff command or the specified number of flashes for lmFlash) the previous lmBlinkOn mode will be automatically restored (until it was canceled by the lmBlinkOff command)


## Example command sequence:

lmOn:                 LED is on

lmOff:                LED off

lmFlash 3 100 500:    the LED will blink three times with a duration of 100 ms with a pause between flashes of 500 ms, after which it will go out

lmBlinkOn 2 100 5000: LED starts blinking in a series of 2 flashes with a duration of 100 ms and a pause between bursts of 5000 ms

lmOn:                 LED on, blinking temporarily suspended

lmOff:                return to the last blinking mode (in a series of 2 blinks for 100 ms after 5 seconds)

lmFlash 30 100 500:   the LED will blink 30 times with a duration of 100 ms with a pause between flashes of 500 ms, after which we automatically return to the last blinking mode (in a series of 2 flashes of 100 ms each after 5 seconds)

lmOff:                nothing has changed ;-)

lmBlinkOn 1 500 500:  change of blinking mode - continuous uniform blinking 0.5 s on / 0.5 s off

lmBlinkOff:           disable blinking mode, off


## Dependencies:
  - https://github.com/kotyara12/rLog

### Notes:
  - libraries starting with the <b>re</b> prefix are only suitable for ESP32 and ESP-IDF
  - libraries starting with the <b>ra</b> prefix are only suitable for ARDUINO compatible code
  - libraries starting with the <b>r</b> prefix can be used in both cases (in ESP-IDF and in ARDUINO)