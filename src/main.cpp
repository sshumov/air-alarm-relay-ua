#include "main.h"
#include <TaskScheduler.h>
#include <Wire.h>
#include <FS.h>
#include <FTPServer.h>
#include "buttom.h"
#include "web.h"
#include "alert.h"
#include "timer.h"
#include "relay.h"


#define FW_PATH "/firmware.bin"


DNSServer               dns;
WebServer               Wserver(80);
WiFiSettingsClass       WiFiSettings(&Wserver,&dns);

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

FS* fileSystem = &LittleFS;
const char* fsName = "LittleFS";

FTPServer ftpSrv(LittleFS);

RtcMemory rtcMemory;
RTCdata* RTCDATA;
Scheduler SCHEDULER;                  // Системный шедулер

//bool safe_mode;                       // Режим загрузки Soft AP
bool fsOk;                            // статус файловой системы
bool wifi_status;

/*
Relay control function
*/


// --------------------------------------------------------------------------------


void setup() {

DBG_OUTPUT_PORT.begin(115200);
uint  rtcst;
rtcst = rtcMemory.begin();
RTCDATA = rtcMemory.getData<RTCdata>();
Wire.begin();

if(rtcst) {
      print_DEBUG(F("\n\rRTC: Initialization done!\n\r"));
  } else {
      print_DEBUG(F("\n\rRTC: No previous data found. The memory is reset to zeros!\n\r"));
      RTCDATA->safe_mode = false;
      relay_off(0);
}

DTM = RTC.now();rtc_time=DTM.unixtime(); // GET RTC time
print_DEBUG("Unixtime RTC: " + String(rtc_time));

fsOk = ESPFS.begin();
if(fsOk == false ) {
  print_DEBUG(F("Filesystem init failed!\n\rFormatted FS and reboot"));
  ESPFS.format();
  ESP.restart();
} else {
#ifdef UPDATE_FROM_FS
if(ESPFS.exists(FW_PATH)) {
  File Ufile = ESPFS.open(FW_PATH,"r");
  if(Ufile) {
    print_DEBUG("Find firmware update from FS");
    size_t fileSize = Ufile.size();
    if(Update.begin(fileSize)){
      print_DEBUG("Starting update...");
      Update.writeStream(Ufile);
      if(Update.end()){
        print_DEBUG("Successful update");
        delay(1000);
        Ufile.close();
        ESPFS.remove(FW_PATH);
        ESP.restart();
      } else {
      Serial.println("Error Occurred: " + String(Update.getError()));
      }
  }
}
  Ufile.close();
  ESPFS.remove(FW_PATH);
}  
#endif
}
if (WiFi.getAutoReconnect() != true)  WiFi.setAutoReconnect(true);

gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    wifi_status = true;print_DEBUG("WIFI ON");
  });

disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    if(wifi_status == true ) {
            wifi_status = false;print_DEBUG("WIFI OFF");
    }
  });

SCHEDULER.init();

conf_relay_cron  = WiFiSettings.string( "relay_cron",24, "000000000000000000000000","Relay hour activity");
conf_utc_offset = WiFiSettings.integer( "utc_off",10800,"+UTC offset in sec");
conf_alert      = WiFiSettings.checkbox("air_alert",false,"Check AIR alert");
conf_alert_url  = WiFiSettings.string( "Alert URL", "https://alerts.com.ua/","URL alert API");
conf_region_id  = WiFiSettings.string( "region_id", "17","region Id");
conf_api_key    = WiFiSettings.string( "api_key", "XXXX","API key");
conf_alert_get_interval = WiFiSettings.integer( "alert_int",60,"Check alert interval in sec");

if(RTCDATA->safe_mode == true ) {
  print_DEBUG(F("Detected safe mode boot"));  
  RTCDATA->safe_mode = false;
  rtcMemory.save();
  WiFiSettings.AP = true;
  WiFiSettings.portal();
}

WiFiSettings.AP = false;
WiFiSettings.connect(false, 5);
WiFiSettings.portal();

init_timer(&SCHEDULER);
init_buttom(&SCHEDULER);
init_alert(&SCHEDULER);
init_relay(&SCHEDULER);
init_web(&SCHEDULER);

print_DEBUG("admin passwd: " + String(WiFiSettings.password.c_str()));

ftpSrv.begin(F("admin"), WiFiSettings.password.c_str());

}

void loop() {
  SCHEDULER.execute();
  ftpSrv.handleFTP();
  delay(1);
}

