#include "main.h"
#include "timer.h"

DS3231 RTCClock;
RTClib RTC;
DateTime DTM;
uint32_t rtc_time; // DS3231 time

Task TIMER_task(1000, TASK_FOREVER, &timer_task_callback);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org");

uint16_t conf_utc_offset;


void init_timer(Scheduler *sc) {
  print_DEBUG("Timer init ...");
  sc->addTask(TIMER_task);
  TIMER_task.enable();
}

void timer_task_callback(void) {
  if(TIMER_task.isFirstIteration()) {
      timeClient.setUpdateInterval(3600);
      timeClient.setTimeOffset(conf_utc_offset);
      timeClient.begin();
      print_DEBUG("Start NTP task");

    }
    timeClient.update();
}
