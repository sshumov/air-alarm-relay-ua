#include <asyncHTTPrequest.h>
#include <ArduinoJson.h>

#include "main.h"
#include "alert.h"
#include "relay.h"

String conf_alert_url;
String conf_region_id;
String conf_api_key;
bool   conf_alert;
uint16_t  conf_alert_get_interval;

bool alert_status;

Task ALERT_task(0, TASK_FOREVER, &alert_task_callback);

StaticJsonDocument<2048> jsonBuffer;

asyncHTTPrequest request;
String api_url;

void alert_task_callback() {
    if(wifi_status == true ) {
        print_DEBUG("Check AIR alert ");
        if(request.readyState() == 0 || request.readyState() == 4){
            request.open("GET", api_url.c_str());
            request.setReqHeader("X-API-Key",conf_api_key.c_str());
            request.send();
        }
    }
}

void requestCB(void* optParm, asyncHTTPrequest* request, int readyState){
    String resp;

    if(readyState == 4){
        if(request->responseHTTPcode() == 200 ) {
            resp = request->responseText();
            print_DEBUG(resp);
            DeserializationError error = deserializeJson(jsonBuffer, resp);
            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
            } else {
                alert_status = jsonBuffer["state"]["alert"];
                if(alert_status == false) {
                    print_DEBUG("NO ALERT");
                } else {
                    print_DEBUG("!!! ALERT !!!");
                    relay_off(0);
                }
            }
        } else {
            print_DEBUG("Error response from AIR alert site, turn off relay");
            alert_status = true;
        }

    }
}

void init_alert(Scheduler *sc) {
    print_DEBUG("Alert Task init ...");
    if(conf_alert == true) {
        print_DEBUG("AIR alert ON");
        print_DEBUG("Region ID: " + conf_region_id + " API key: " + conf_api_key);
        alert_status = true;
        api_url = conf_alert_url + conf_region_id;
        request.onReadyStateChange(requestCB);
        sc->addTask(ALERT_task);
        ALERT_task.setInterval(conf_alert_get_interval * 1000);
        ALERT_task.enable();

#ifdef DEBUG
    request.setDebug(true);
#endif
    } else {
        alert_status = false;
    }
}
