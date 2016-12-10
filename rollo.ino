#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <AccelStepper.h>

const char* wlan_ssid = "hsmr";
const char* wlan_psk = "xxx";
const char* host_name = "rollo";
const char* ota_password = "xxx";

const char* password = "xxx";
const int travelling_distance = 1750 * -1;

const int stepper_pin_enable = 13;  // D7
const int stepper_pin_step = 12;  // D6
const int stepper_pin_dir = 14;  // D5

const int toggle_button_pin = 5;  // D1

ESP8266WebServer server(80);

AccelStepper stepper(AccelStepper::DRIVER, stepper_pin_step, stepper_pin_dir);

String toggle() {
  if (stepper.currentPosition() == travelling_distance) {
    stepper.moveTo(0);
    return "moving to 0... (up)";
  }
  else if (stepper.currentPosition() == 0) {
    stepper.moveTo(travelling_distance);
    return "moving to " + String(travelling_distance) + "... (down)";
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wlan_ssid, wlan_psk);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname(host_name);
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  pinMode(stepper_pin_enable, OUTPUT);
  digitalWrite(stepper_pin_enable, HIGH);

  stepper.setMaxSpeed(340.0);
  stepper.setAcceleration(500.0);
  stepper.moveTo(travelling_distance);  // FIXME: you spin my roll right round, right round, when you go down, when you go down, down... use endstop

  pinMode(toggle_button_pin, INPUT);

  server.on("/toggle", [](){
    if (server.arg("password") == password) {
        server.send(200, "text/plain", toggle());
    } else {
      server.send(401, "text/plain", "u shall not pass!");
    }
  });

  server.begin();
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();

  if (stepper.distanceToGo() == 0) {
    digitalWrite(stepper_pin_enable, HIGH);
    server.handleClient();  // avoid timing problems
    if (digitalRead(toggle_button_pin) == HIGH) {
      toggle();
    }
  } else {
    digitalWrite(stepper_pin_enable, LOW);
  }
  stepper.run();
}
