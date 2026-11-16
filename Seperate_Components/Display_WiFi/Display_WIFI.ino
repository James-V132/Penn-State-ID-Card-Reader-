#include <SoftwareSerial.h>

SoftwareSerial espSerial(8, 9); // RX, TX

// Arduino RX = Pin 8 -> TX ESP8266
// RX ESP8266 -> Arduino TX = Pin 9

String SSID = "JAMES-iPHONE (2)";
String PASS = "Passwording";

void setup() {
  Serial.begin(9600);   // To PC
  espSerial.begin(9600); // To ESP8266 (match your ESP's baud rate)

  Serial.println("Connecting to WiFi...");
  sendAT("AT+RST");
  sendAT("AT+CWMODE=1");
  sendAT("AT+CWJAP=\"" + SSID + "\",\"" + PASS + "\"");
  sendAT("AT+CIFSR");
}

void loop() {
  if (espSerial.available()) { Serial.write(espSerial.read()); }
  if (Serial.available()) { espSerial.write(Serial.read()); }
}

void sendAT(String command) {
  espSerial.println(command);
}

// AT+CWLAP = WiFi list