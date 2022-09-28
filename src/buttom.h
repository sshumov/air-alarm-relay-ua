#ifndef _BUTTOM_
#define _BUTTOM_
#define PRESS_NONE   0
#define PRESS_SHORT  1
#define PRESS_LONG   2
#define DELAY_BUTTON_ACTION1  80             // Затримка нажатої кномки для виконання дії 1 зазначено у мс
#define DELAY_BUTTON_ACTION2  5000           // Затримка нажатої кномки для виконання дії 2 зазначено у мс
#define BUTTON_PIN            0              // Номер GPIO ESP32 використовуємий для кнопки

#include "main.h"
#include "rtc_memory.h"
#include <TaskSchedulerDeclarations.h>

void buttom_task_callback(void);
void init_buttom(Scheduler *sc);
extern uint32_t  timeButtonDown;
extern uint8_t   xPressButton_action;
#endif
