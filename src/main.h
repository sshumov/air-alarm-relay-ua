#include <Arduino.h>
#include <ArduinoOTA.h>
#include "DS3231.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "WifiSettings.h"

#include <Wire.h>
#include <WiFiUdp.h>
#include "rtc_memory.h"

#define WebServer ESP8266WebServer

#define DBG_OUTPUT_PORT Serial
#include <LittleFS.h>
#define ESPFS LittleFS
#define GET_CHIPID()  (ESP.getChipId())

#define     LED_GREEN             16             // Номер GPIO  використовуємий для ввімкненя зеленого діоду
#define     LED_RED               03             // Номер GPIO  використовуємий для ввімкненя червоного діоду
#define     LED_BLUE              02             // Номер GPIO  використовуємий для ввімкненя блакитного діоду
#define     RELAY_ON_PIN          13 // Номер пина для реле "защелки"

#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_STATUS_REQUEST
#define _TASK_TIMECRITICAL

#include <TaskSchedulerDeclarations.h>

void print_DEBUG(String msg);
// void ntp_task_callback(void);

extern Scheduler      SCHEDULER;      // Task scheduler

extern bool           safe_mode;      // boot safe mode
extern bool           fsOk;           // filesystem is ok ?

extern WebServer      Wserver;


extern const char*    fsName;
extern FS*            fileSystem;

#ifndef _RTC_D_
#define _RTC_D_
typedef struct {
  bool  safe_mode;
  uint32_t relay_state;
} RTCdata;
#endif
extern bool             wifi_status;
extern RTCdata*         RTCDATA;
extern RtcMemory        rtcMemory;


