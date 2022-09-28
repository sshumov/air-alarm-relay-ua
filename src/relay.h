#ifndef _RELAY_
#define _RELAY_

#include "main.h"
#include <TaskSchedulerDeclarations.h>

extern String conf_relay_cron;
void relay_on(uint num);
void relay_off(uint num);
void relay_task_callback(void);
void init_relay(Scheduler *sc);
#endif


