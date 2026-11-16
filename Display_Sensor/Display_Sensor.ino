#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Wire.h>
#include <SoftwareSerial.h>

SoftwareSerial espSerial(2, 3);
String SSID = "JAMES-iPHONE (2)";
String PASS = "Passwording";
String ip = "172.20.10.2";
int port = 3000;
String loc = "http://" + ip + ":" + String(port);

LiquidCrystal lcd(A5, A4, A3, A2, A1, A0);

// Structure for API
struct ApiStruct {
  bool err;
  String data;
};

#define SCK  (13)   
#define MOSI (11)   
#define MISO (12)   
#define SS   (10) 

Adafruit_PN532 nfc(SS);

void setup() 
{
  Serial.begin(9600);
  Serial.println(F("System Booting..."));
  lcd.begin(16,2);
  lcd.setCursor(0, 0);
  lcd.print("Please Scan...");
  
  SPI.begin();
  delay(1000);
  nfc.begin();
  nfc.setPassiveActivationRetries(0x01);

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    lcd.clear();
    lcd.print("NFC Not Found!");
    while (1); 
  }
  
  espSerial.begin(115200);
  sendAT("AT+UART_DEF=9600,8,1,0,0");
  espSerial.begin(9600);

  lcd.clear();
  lcd.print("WiFi Setup...");
  
  sendAT("AT+RST");
  delay(2000);
  sendAT("AT+CWMODE=1");
  delay(500);
  sendAT("AT+CWJAP=\"" + SSID + "\",\"" + PASS + "\"");
  delay(8000);
  
  sendAT("AT+CIFSR");
  delay(2000);
  
  lcd.clear();
  lcd.print("WiFi OK");
  delay(1000);
  
  sendAT("AT+CIPMUX=0");
  delay(200);
  
  lcd.clear();
  lcd.print("Ready!");
}

void drainSerial() {
  while (espSerial.available()) {
    espSerial.read();
  }
  delay(100);
}

void sendAT(String command) {
  Serial.print(F("> ")); Serial.println(command);
  espSerial.println(command);
  delay(500);
}

void memDump(bool success){
  uint8_t data[16];
  for (currentblock = 0; currentblock < 64; currentblock++)
      {
        // Check if this is a new block so that we can reauthenticate
        if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

        // If the sector hasn't been authenticated, do so first
        if (!authenticated)
        {
          // Starting of a new sector ... try to to authenticate
          Serial.print("------------------------Sector ");Serial.print(currentblock/4, DEC);Serial.println("-------------------------");
          if (currentblock == 0)
          {
              // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
              // or 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 for NDEF formatted cards using key a,
              // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
              success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
          }
          else
          {
              // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
              // or 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 for NDEF formatted cards using key a,
              // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
              success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
          }
          if (success)
          {
            authenticated = true;
          }
          else
          {
            Serial.println("Authentication error");
          }
        }
        // If we're still not authenticated just skip the block
        if (!authenticated)
        {
          Serial.print("Block ");Serial.print(currentblock, DEC);Serial.println(" unable to authenticate");
        }
        else
        {
          // Authenticated ... we should be able to read the block now
          // Dump the data into the 'data' array
          success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
          if (success)
          {
            // Read successful
            Serial.print("Block ");Serial.print(currentblock, DEC);
            if (currentblock < 10)
            {
              Serial.print("  ");
            }
            else
            {
              Serial.print(" ");
            }
            // Dump the raw data
            nfc.PrintHexChar(data, 16);
          }
          else
          {
            // Oops ... something happened
            Serial.print("Block ");Serial.print(currentblock, DEC);
            Serial.println(" unable to read this block");
          }
        }
      }
}

bool waitForResponse(const String &target, uint32_t timeoutMs = 3000) {
  uint32_t start = millis();
  String response;
  while (millis() - start < timeoutMs) {
    while (espSerial.available()) {
      char c = espSerial.read();
      response += c;
      Serial.write(c);
    }
    if (response.indexOf(target) >= 0) {
      Serial.print(F("\n[Found: ")); Serial.print(target); Serial.println(F("]"));
      return true;
    }
    delay(10);
  }
  Serial.print("\n[Timeout waiting for: "); Serial.print(target); Serial.println(F("]"));
  Serial.print("[Got: "); Serial.print(response); Serial.println(F("]"));
  return false;
}

String uidToString(uint8_t *uid, uint8_t len) {
  String out;
  for (uint8_t i = 0; i < len; i++) {
    if (uid[i] < 0x10) out += "0";
    out += String(uid[i], HEX);
  }
  out.toUpperCase();
  return out;
}

// API function
struct ApiStruct callApi(String method, String path, String body = "") {
  ApiStruct resp;
  resp.err = true;
  resp.data = "";

  // 1. Force close any existing hanging connection
  espSerial.println(F("AT+CIPCLOSE"));
  delay(200);
  drainSerial();

  espSerial.println(F("AT+CIPCLOSE")); 
  delay(500);

  // 2. Start the connection - Clean single line
  delay(100);
  String connectCmd = "AT+CIPSTART=\"TCP\",\"172.20.10.2\",3000";
  espSerial.println(connectCmd);
  
  // Docker needs a bit of time to "handshake"
  if (!waitForResponse("OK", 5000) && !waitForResponse("ALREADY CONNECT", 5000)) {
    // If it still fails, try sending AT+CIPCLOSE first to reset the port
    espSerial.println(F("AT+CIPCLOSE")); 
    delay(500);
    Serial.println(F("[ERROR] CIPSTART failed - check if Docker is running"));
    resp.data = "1";
    return resp;
  }

  // 3. Build the HTTP Request
  String request = "GET " + path + " HTTP/1.1\r\n";  
  request += "Host: 172.20.10.2\r\n"; // No "http://" here, just the IP
  request += "Connection: close\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + String(body.length()) + "\r\n";
  request += "\r\n";
  request += body;

  // 4. Send the data
  espSerial.print(F("AT+CIPSEND="));
  espSerial.println(request.length());
  
  if (waitForResponse(">", 5000)) {
    espSerial.print(request);
  } else {
    resp.data = "2";
    return resp;
  }

  if (waitForResponse("SEND OK", 5000)) {
    // Capture the response
    uint32_t t0 = millis();
    String raw;
    while (millis() - t0 < 5000) {
      while (espSerial.available()) {
        raw += (char)espSerial.read();
        t0 = millis();
      }
    }
    int bodyIdx = raw.indexOf("\r\n\r\n");
    if (bodyIdx >= 0) {
      resp.data = raw.substring(bodyIdx + 4);
      resp.err = false;
    }
  }
  return resp;
}

void loop() {
  // 1. Check if the ESP8266 sent anything back (to prevent buffer overflow)
  while (espSerial.available()) 
  {
    Serial.write(espSerial.read()); 
  }

  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength = 0;   // Length of the UID (4 or 7 bytes depending on the card type)

  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) 
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("UID Found:");
    lcd.setCursor(0, 1);
    
    for (uint8_t i = 0; i < uidLength; i++) 
    {
      lcd.print(uid[i], HEX);
      if (i < uidLength - 1) lcd.print(" ");
    }

    memDump(success);

    // Call API
    String cardId = uidToString(uid, uidLength);
    delay(100);
    ApiStruct apiResp = callApi("GET", "/api/getIsKnown?cardId=" + cardId);

    if (apiResp.err) {
      lcd.print("API error #");
      lcd.print(apiResp.data);
      Serial.print("ERROR CODE: "); Serial.println(apiResp.data);
      delay(3000);
    } else {
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, apiResp.data);
      if (error) {
        lcd.print("JSON error");
      } else {
        bool known = doc["known"] | false;
        if (known && !doc["user"].isNull()) {
          String name;
          if (doc["user"]["firstname"]) name += String(doc["user"]["firstname"].as<const char*>());
          if (doc["user"]["lastname"]) {
            if (name.length()) name += " ";
            name += String(doc["user"]["lastname"].as<const char*>());
          }
          lcd.print(name.length() ? name : "Known user");
        } else {
          lcd.print("Type info on dashboard");
          // register as unknown scan: use POST JSON to match server endpoint
          ApiStruct postResp = callApi("POST", "/api/postUnknownScan", "{\"cardId\":\"" + cardId + "\"}");
          if (postResp.err) {
            Serial.println(F("POST unknown scan failed"));
          } else {
            Serial.println("POST unknown scan success: " + postResp.data);
          }
        }
      }
    }

    delay(3000); // Give time to read the screen
    lcd.clear();
    lcd.print("Please Scan...");
  }

}

// lcd is for display
// nfc is the scanner
// espSerial is for the wifi
