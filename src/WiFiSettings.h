#ifndef WiFiSettings_h
#define WiFiSettings_h

#include <Arduino.h>
#include <functional>
#include "main.h"

#include <DNSServer.h>
#include <limits.h>

#define WIFI_AUTH_OPEN ENC_TYPE_NONE
#define WIFI_AUTH_WPA2_ENTERPRISE -1337 // not available on ESP8266


class WiFiSettingsClass {
    public:
        typedef std::function<void(void)> TCallback;
        typedef std::function<int(void)> TCallbackInt;

        WiFiSettingsClass(WebServer* Wserver,DNSServer* dns);
        void begin();
        bool connect(bool portal = true, int wait_seconds = 30);
        void portal(void); // true - start on AP mode else only web server
        String string(const String& name, const String& init = "", const String& label = "");
        String string(const String& name, unsigned int max_length, const String& init = "", const String& label = "");
        String string(const String& name, unsigned int min_length, unsigned int max_length, const String& init = "", const String& label = "");
        long integer(const String& name, long init = 0, const String& label = "");
        long integer(const String& name, long min, long max, long init = 0, const String& label = "");
        bool checkbox(const String& name, bool init = false, const String& label = "");
        bool AP;

        String hostname;
        String password;
        bool secure;

        TCallback onConnect;
        TCallbackInt onWaitLoop;
        TCallback onSuccess;
        TCallback onFailure;
        TCallback onPortal;
        TCallback onConfigSaved;
        TCallback onRestart;
        TCallback onPortalWaitLoop;
        
    private:
        bool begun;
        WebServer* _server;
        DNSServer* _dnsServer;
        
};

extern WiFiSettingsClass WiFiSettings;

#endif
