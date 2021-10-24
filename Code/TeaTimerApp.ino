#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <iostream>
#include <string>
// #include <ESP8266WiFi.h>  // macht immer noch Probleme ...
// #include <PubSubClient.h>

#include "include/TeaTimer.h"  // WTF es darf nicht im selben Verzeichnis sein???

// State of the system
bool SERIAL_ACTIVE{false};
SerialTempInput dataInput;
SerialInterface *serial;

// the setup function is run once after startup
void setup() {
    serial = new SerialInterface(); // Serial.begin etc. inside constructor
    dataInput = SerialTempInput(serial);
}

// the loop function is looped indefinitely after setup
void loop() {
    static double T;
    serial->handleSerial();
    T = dataInput.getTemperature();
    delay(2000);
    serial->write_str(String(T));
}
