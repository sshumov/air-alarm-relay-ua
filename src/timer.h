#ifndef _TIMER_
#define _TIMER_
#define PRESS_NONE   0
#define PRESS_SHORT  1
#define PRESS_LONG   2
#define DELAY_BUTTON_ACTION1  80             // Затримка нажатої кномки для виконання дії 1 зазначено у мс
#define DELAY_BUTTON_ACTION2  5000           // Затримка нажатої кномки для виконання дії 2 зазначено у мс
#define BUTTON_PIN            0              // Номер GPIO ESP32 використовуємий для кнопки

#include "main.h"
#include <TaskSchedulerDeclarations.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

extern NTPClient timeClient;
extern DS3231 RTCClock;
extern RTClib RTC;
extern DateTime DTM;
extern uint32_t rtc_time; // DS3231 time
extern uint16_t conf_utc_offset;
void timer_task_callback(void);
void init_timer(Scheduler *sc);
#endif

