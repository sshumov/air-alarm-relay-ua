#include "WiFiSettings.h"
#include <ESP8266httpUpdate.h>
#include <ArduinoOTA.h>
#include <vector>

#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })

namespace {  // Helpers
    String slurp(const String& fn) {
        //String cnf="/wifi" + fn + ".txt";
        File f = ESPFS.open("/sys" + fn + ".txt", "r");
        //File f = ESPFS.open(fn, "r");
        String r = f.readString();
        f.close();
        return r;
    }

    void spurt(const String& fn, const String& content) {
        //String cnf="/wifi" + fn + ".txt";
        File f = ESPFS.open("/sys" + fn + ".txt", "w");
        //File f = ESPFS.open(fn, "w");
        f.print(content);
        f.close();
    }

    String pwgen() {
        const char* passchars = "ABCEFGHJKLMNPRSTUXYZabcdefhkmnorstvxz23456789-#@%^";
        String password = "";
        for (int i = 0; i < 16; i++) {
            // Note: no seed needed for ESP8266 and ESP32 hardware RNG
            password.concat( passchars[random(strlen(passchars))] );
        }
        return password;
    }

    String html_entities(const String& raw) {
        String r;
        for (unsigned int i = 0; i < raw.length(); i++) {
            char c = raw.charAt(i);
            if (c >= '!' && c <= 'z' && c != '&' && c != '<' && c != '>' && c != '\'' && c != '"') {
                // printable non-whitespace ascii minus html and {}
                r += c;
            } else {
                r += Sprintf("&#%d;", raw.charAt(i));
            }
        }
        return r;
    }

    struct WiFiSettingsParameter {
        String name;
        String label;
        String value;
        String init;
        long min = LONG_MIN;
        long max = LONG_MAX;

        String filename() { String fn = "/"; fn += name; return fn; }
        virtual void store(const String& v) { value = v; spurt(filename(), v); }
        void fill() { value = slurp(filename()); }
        virtual String html() = 0;
    };

    struct WiFiSettingsString : WiFiSettingsParameter {
        String html() {
            String h = F("<label>{label}:<br><input name='{name}' value='{value}' placeholder='{init}' minlength={min} maxlength={max}></label>");
            h.replace("{name}", html_entities(name));
            h.replace("{value}", html_entities(value));
            h.replace("{init}", html_entities(init));
            h.replace("{label}", html_entities(label));
            h.replace("{min}", String(min));
            h.replace("{max}", String(max));
            return h;
        }
    };

    struct WiFiSettingsInt : WiFiSettingsParameter {
        String html() {
            String h = F("<label>{label}:<br><input type=number step=1 min={min} max={max} name='{name}' value='{value}' placeholder='{init}'></label>");
            h.replace("{name}", html_entities(name));
            h.replace("{value}", html_entities(value));
            h.replace("{init}", html_entities(init));
            h.replace("{label}", html_entities(label));
            h.replace("{min}", String(min));
            h.replace("{max}", String(max));
            return h;
        }
    };

    struct WiFiSettingsBool : WiFiSettingsParameter {
        String html() {
            String h = F("<label><input type=checkbox name='{name}' value=1{checked}> {label} (default: {init})</label>");
            h.replace("{name}", html_entities(name));
            h.replace("{checked}", value.toInt() ? " checked" : "");
            h.replace("{init}", init.toInt() ? "&#x2611;" : "&#x2610;");
            h.replace("{label}", html_entities(label));
            return h;
        }
        virtual void store(String v) {
            value = v.length() ? "1" : "0";
            spurt(filename(), value);
        }
    };

    struct std::vector<WiFiSettingsParameter*> params;
}

String WiFiSettingsClass::string(const String& name, const String& init, const String& label) {
    begin();
    struct WiFiSettingsString* x = new WiFiSettingsString();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = init;
    x->fill();

    params.push_back(x);
    return x->value.length() ? x->value : x->init;
}

String WiFiSettingsClass::string(const String& name, unsigned int max_length, const String& init, const String& label) {
    String rv = string(name, init, label);
    params.back()->max = max_length;
    return rv;
}

String WiFiSettingsClass::string(const String& name, unsigned int min_length, unsigned int max_length, const String& init, const String& label) {
    String rv = string(name, init, label);
    params.back()->min = min_length;
    params.back()->max = max_length;
    return rv;
}

long WiFiSettingsClass::integer(const String& name, long init, const String& label) {
    begin();
    struct WiFiSettingsInt* x = new WiFiSettingsInt();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = init;
    x->fill();

    params.push_back(x);
    return (x->value.length() ? x->value : x->init).toInt();
}

long WiFiSettingsClass::integer(const String& name, long min, long max, long init, const String& label) {
    long rv = integer(name, init, label);
    params.back()->min = min;
    params.back()->max = max;
    return rv;
}

bool WiFiSettingsClass::checkbox(const String& name, bool init, const String& label) {
    begin();
    struct WiFiSettingsBool* x = new WiFiSettingsBool();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = String((int) init);
    x->fill();

    // Apply default immediately because a checkbox has no placeholder to
    // show the default, and other UI elements aren't sufficiently pretty.
    if (! x->value.length()) x->value = x->init;

    params.push_back(x);
    return x->value.toInt();
}

void WiFiSettingsClass::portal() {
    String tmp;

    static int num_networks = -1;
    begin();
    if(AP == true ) {
#ifdef ESP32
        WiFi.disconnect(true, true);    // reset state so .scanNetworks() works
#else
        WiFi.disconnect(true);
#endif
    
        if (secure && password.length()) {
         WiFi.softAP(hostname.c_str(), password.c_str());
        } else {
         WiFi.softAP(hostname.c_str());
        }
        
        delay(500);
        _dnsServer->setTTL(0);
        _dnsServer->start(53, "*", WiFi.softAPIP());
        Serial.println(WiFi.softAPIP().toString());
   }
    

    if (onPortal) onPortal();
        if(AP == true) {
            _server->on("/", HTTP_GET, [this]() {
            _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
            _server->send(200, "text/html");
//            _server->sendContent(F("<!DOCTYPE html>\n<meta charset=UTF-8><title>"));
            _server->sendContent(F("<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>"
            "<title>Hello!</title></head><body>Go to <a href='portal'>configure page</a> to change settings.</body></html>\n"
          )); });
        }

        _server->on("/portal", HTTP_GET, [this]() {
            if(AP == false && secure == true )
                if (!_server->authenticate("admin", password.c_str()))
                        return _server->requestAuthentication();
        _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        _server->send(200, "text/html");
        _server->sendContent(F("<!DOCTYPE html>\n<meta charset=UTF-8><title>"));
        _server->sendContent(html_entities(hostname));
        _server->sendContent(F("</title>"
            "<meta name=viewport content='width=device-width,initial-scale=1'>"
            "<style>*{box-sizing:border-box} "
            "html{background:#444;font:10pt sans-serif}"
            "body{background:#ccc;color:black;max-width:30em;padding:1em;margin:1em auto}"
            "a:link{color:#000} label{clear:both}"
            "select,input:not([type^=c]){display:block;width:100%;border:1px solid #444;padding:.3ex}"
            "input[type^=s]{width:auto;background:#de1;padding:1ex;border:1px solid #000;border-radius:1ex}"
            "[type^=c]{float:left}:not([type^=s]):focus{outline:2px solid #d1ed1e}"
            "</style><form action=/restart method=post>Hello, my name is "
        ));

        _server->on("/portal", HTTP_POST, [this]() {
            spurt("/wifi-ssid", _server->arg("ssid"));
            String pw = _server->arg("password");
            if (pw != "##**##**##**") {
                spurt("/wifi-password", pw);
            }

            for (auto p : params) p->store(_server->arg(p->name));

            _server->sendHeader("Location", "/portal");
            _server->send(302, "text/plain", "ok");
            if (onConfigSaved) onConfigSaved();
        });
        
        _server->sendContent(html_entities(hostname));
        _server->sendContent(F(
            ".<input type=submit value='Restart device'></form><hr>"
            "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"
            "<h2>Configuration</h2><form method=post><label>SSID:<br><b class=s>Scanning for WiFi networks...</b>"
        ));

        if (num_networks < 0) num_networks = WiFi.scanNetworks();
        Serial.printf("%d WiFi networks found.\n", num_networks);

        _server->sendContent(F(
            "<style>.s{display:none}</style><select name=ssid onchange=\"document.getElementsByName('password')[0].value=''\">"
        ));

        String current = slurp("/wifi-ssid");
        bool found = false;
        for (int i = 0; i < num_networks; i++) {
            String opt = F("<option value='{ssid}'{sel}>{ssid} {lock} {1x}</option>");
            String ssid = WiFi.SSID(i);
            uint8_t mode = WiFi.encryptionType(i);

            opt.replace("{sel}",  ssid == current && !found ? " selected" : "");
            opt.replace("{ssid}", html_entities(ssid));
            opt.replace("{lock}", mode != WIFI_AUTH_OPEN ? "&#x1f512;" : "");
            opt.replace("{1x}",   mode == WIFI_AUTH_WPA2_ENTERPRISE ? "(won't work: 802.1x is not supported)" : "");
            _server->sendContent(opt);

            if (ssid == current) found = true;
        }
        if (!found && current.length()) {
            String opt = F("<option value='{ssid}' selected>{ssid} (&#x26a0; not in range)</option>");
            opt.replace("{ssid}", html_entities(current));
            _server->sendContent(opt);
        }

        _server->sendContent(F("</select></label><a href=/rescan onclick=\"this.innerHTML='scanning...';\">rescan</a><p><label>WiFi WEP/WPA password:<br><input name=password value='"
        ));
        if (slurp("/wifi-password").length()) _server->sendContent("##**##**##**");
        _server->sendContent(F("'></label><hr>"));

        for (auto p : params) {
            _server->sendContent(p->html());
            _server->sendContent("<p>");
        }

        _server->sendContent(F("<input type=submit value=Save></form>"));
    });

    _server->on("/restart", HTTP_POST, [this]() {
        _server->send(200, "text/plain", "Doei!");
        if (onRestart) onRestart();
        ESP.restart();
    });

    _server->on("/rescan", HTTP_GET, [this]() {
        _server->sendHeader("Location", "/");
        _server->send(302, "text/plain", "wait for it...");
        num_networks = WiFi.scanNetworks();
    });
    _server->on("/update", HTTP_POST, [this]() {
      _server->sendHeader("Connection", "close");
      _server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, [this]() {
      HTTPUpload& upload = _server->upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
#ifdef ESP8266
        WiFiUDP::stopAll();
#endif
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
 
    _server->onNotFound([this]() {
        _server->sendHeader("Location", "http://" + hostname + "/");
        _server->send(302, "text/plain", "hoi");
    });

   if(AP == true ) {
    _server->begin();
    for (;;) {
        _server->handleClient();
        _dnsServer->processNextRequest();
        if (onPortalWaitLoop) onPortalWaitLoop();
        wdt_reset();
        yield();
    }
   }
}

bool WiFiSettingsClass::connect(bool portal, int wait_seconds) {
    begin();

    WiFi.mode(WIFI_STA);

    String ssid = slurp("/wifi-ssid");
    String pw = slurp("/wifi-password");
    if (ssid.length() == 0 && portal) {
        Serial.println("First contact!\n");
        this->portal();
    }
    #ifdef DEBUG
        Serial.printf("Connecting to WiFi SSID '%s'", ssid.c_str());
    #endif
    if (onConnect) onConnect();

    WiFi.hostname(hostname.c_str());
    WiFi.begin(ssid.c_str(), pw.c_str());

    unsigned long starttime = millis();
    while (WiFi.status() != WL_CONNECTED && (wait_seconds < 0 || (millis() - starttime) < wait_seconds * (uint)1000)) {
        Serial.print(".");
        delay(onWaitLoop ? onWaitLoop() : 100);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(" failed.");
        if (onFailure) onFailure();
        if (portal) { AP = true; this->portal(); }
        return false;
    }

    Serial.println(WiFi.localIP().toString());
    if (onSuccess) onSuccess();
    return true;
}

void WiFiSettingsClass::begin() {
    if (begun) return;
    begun = true;

    if (!password.length()) {
        password = string(
            "WiFiSettings-password",
            8, 63,
            "",
            F("WiFi password for the configuration portal")
        );
        if (password == "") {
            // With regular 'init' semantics, the password would be changed
            // all the time.
            password = pwgen();
            params.back()->store(password);
        }
    }

    if (!secure) {
        secure = checkbox(
            "WiFiSettings-secure",
            false,
            F("Protect the configuration portal and debug panel")
        );
    }

}

WiFiSettingsClass::WiFiSettingsClass(WebServer* Wserver,DNSServer* dns) {
    hostname = Sprintf("esp8266%06" PRIx32, ESP.getChipId());
    _server= Wserver;
    _dnsServer = dns;
}

