#include "main.h"
#include "relay.h"
#include "alert.h"
#include "timer.h"

Task RELAY_task(60000, TASK_FOREVER, &relay_task_callback);

String conf_relay_cron;
byte relay_buf[24];


void relay_on(uint num) {
    if(RTCDATA->relay_state == false ) {
        digitalWrite(LED_BLUE, LOW);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(RELAY_ON_PIN, LOW);
        digitalWrite(LED_GREEN, HIGH);
        delayMicroseconds(20);
        digitalWrite(RELAY_ON_PIN, HIGH);
        delayMicroseconds(20);
        digitalWrite(RELAY_ON_PIN, LOW);
        RTCDATA->relay_state = true;
        rtcMemory.save();
        print_DEBUG("RELAY ON");
    }
}

void relay_off(uint num) {
if(RTCDATA->relay_state == true ) {
        digitalWrite(LED_BLUE, HIGH);
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(RELAY_ON_PIN, LOW);
        digitalWrite(LED_GREEN, LOW);
        delayMicroseconds(20);
        digitalWrite(RELAY_ON_PIN, HIGH);
        delayMicroseconds(20);
        digitalWrite(RELAY_ON_PIN, LOW);
        RTCDATA->relay_state = false;
        rtcMemory.save();
        print_DEBUG("RELAY OFF");
    }
}




void init_relay(Scheduler *sc) {
    print_DEBUG("Relay init ...");
    relay_off(0);
    conf_relay_cron.getBytes(relay_buf,24);
    sc->addTask(RELAY_task);
    RELAY_task.enable();
}

void relay_task_callback(void) {
    byte hour;
    print_DEBUG("RELAY TASK run");
    if(RELAY_task.isFirstIteration()) {
        print_DEBUG("First run task, skip");

    } else 
    if(alert_status == true ) {
        print_DEBUG("AIR Alert ON, turn off relay");
        relay_off(0);
    } else {
        print_DEBUG("No alert mode");
        hour = timeClient.getHours();
        if(relay_buf[hour] == '1' ) {
            print_DEBUG("Turn ON relay");
            relay_on(0);
        } else {
            print_DEBUG("Turn OFF relay");
            relay_off(0);
        }
    }
}
