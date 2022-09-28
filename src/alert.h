#ifndef _ALERT_
#define _ALERT_
#include "main.h"
#include <TaskSchedulerDeclarations.h>

void alert_task_callback(void);
void init_alert(Scheduler *sc);
extern String conf_alert_url;
extern String conf_region_id;
extern String conf_api_key;
extern bool conf_alert;
extern bool alert_status;
extern uint16_t  conf_alert_get_interval;
#endif

