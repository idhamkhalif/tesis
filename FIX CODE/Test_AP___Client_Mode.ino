#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>
#include "FS.h"
#include "SPIFFS.h"
#include <BlynkSimpleEsp32.h>

#define LED1 25
#define LED2 26
#define LED3 27

/* Set these to your Wifi credentials. */

const char*Wifi_ssid = "roberta2_bawah";                             // SSID of your Router OR mobile hotspot
const char*Wifi_password = "R2bawah2019";                       //  PASSWORD of your Router or Mobile hotspot see below example
const char *auth = "n1CHCyJXFYAUUe3h0dmH5YF1gcv-OkeI";

const char *Apssid = "AC-Automation";                      //give Accesspoint SSID, your esp's hotspot name
const char *Appassword = "123456789";                           //password of your esp's hotspot

const uint16_t kRecvPin = 23;
const uint16_t kIrLedPin = 18;

const uint32_t kBaudRate = 115200;
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;  // Milli-Seconds
const uint16_t kFrequency = 38000;  // in Hz. e.g. 38kHz.

bool Button_off;
bool Button_on;
bool Button_cool;
bool Button_heat;

BLYNK_CONNECTED() {
  Blynk.syncAll();
}

BLYNK_WRITE(V1) {
  if (param.asInt() == 1)
    Button_on = true;
  else
    Button_on = false;
}

BLYNK_WRITE(V2) {
  if (param.asInt() == 1)
    Button_off = true;
  else
    Button_off = false;
}

BLYNK_WRITE(V3) {
  if (param.asInt() == 1)
    Button_cool = true;
  else
    Button_cool = false;
}

BLYNK_WRITE(V4) {
  if (param.asInt() == 1)
    Button_heat = true;
  else
    Button_heat = false;
}

IRsend irsend(kIrLedPin);
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);
decode_results results;

WiFiServer server(80);

const size_t size_j = JSON_ARRAY_SIZE(2400);
DynamicJsonDocument doc(size_j);

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void captureData()
{
  if (irrecv.decode(&results)) {  // We have captured something.

    uint16_t *raw_array = resultToRawArray(&results);
    uint16_t length = getCorrectedRawLength(&results);

    irsend.sendRaw(raw_array, length, kFrequency);
    irrecv.resume();

    // Display a crude timestamp & notification.
    Serial.print(resultToHumanReadableBasic(&results));
    String description = IRAcUtils::resultAcToString(&results);
    if (description.length()) Serial.println("Description: " + description);
    Serial.println(resultToSourceCode(&results));
    yield();
    Serial.println();

    JsonArray data_n = doc.createNestedArray("raw");
    for (int x = 0; x < length; x++) {
      data_n.add(raw_array[x]);
      delay(1);
    }
    digitalWrite(LED2, 0);
    delay(1000);
    digitalWrite(LED2, 1);
  }

  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {                             // if you get a client,
    Serial.println("New Client.");
    String currentLine = "";     // make a String to hold incoming data from the client

    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/on_capture\">here</a> to capture on data RAW.<br>");
            client.print("Click <a href=\"/off_capture\">here</a> to capture off data RAW.<br>");
            client.print("Click <a href=\"/cool_capture\">here</a> to capture cool data RAW<br>");
            client.print("Click <a href=\"/heat_capture\">here</a> to capture heat data RAW.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        if (currentLine.endsWith("GET /on_capture")) {
          File ff_on = SPIFFS.open("/raw_on.json", FILE_WRITE);
          serializeJson(doc, ff_on);
          ff_on.close();

          readFile(SPIFFS, "/raw_on.json");
          listDir(SPIFFS, "/", 0);
          digitalWrite(LED3, 0);
          delay(500);
          digitalWrite(LED3, 1);
          delay(500);
          Serial.println("ON Data Capture");
        }
        if (currentLine.endsWith("GET /off_capture")) {
          File ff_off = SPIFFS.open("/raw_off.json", FILE_WRITE);
          serializeJson(doc, ff_off);
          ff_off.close();

          readFile(SPIFFS, "/raw_off.json");
          listDir(SPIFFS, "/", 0);
          digitalWrite(LED3, 0);
          delay(500);
          digitalWrite(LED3, 1);
          delay(500);
          Serial.println("OFF Data Capture");
        }
        if (currentLine.endsWith("GET /cool_capture")) {
          File ff_cool = SPIFFS.open("/raw_cool.json", FILE_WRITE);
          serializeJson(doc, ff_cool);
          ff_cool.close();

          readFile(SPIFFS, "/raw_cool.json");
          listDir(SPIFFS, "/", 0);
          digitalWrite(LED3, 0);
          delay(500);
          digitalWrite(LED3, 1);
          delay(500);
          Serial.println("COOL Data Capture");
        }
        if (currentLine.endsWith("GET /heat_capture")) {
          File ff_heat = SPIFFS.open("/raw_heat.json", FILE_WRITE);
          serializeJson(doc, ff_heat);
          ff_heat.close();

          readFile(SPIFFS, "/raw_heat.json");
          listDir(SPIFFS, "/", 0);

          Serial.println("HEAT Data Capture");
          Serial.println("Deepsleep Mode");
          digitalWrite(LED3, 0);
          delay(500);
          digitalWrite(LED3, 1);
          delay(500);
          esp_deep_sleep_start();
        }
        yield();  // Or delay(milliseconds); This ensures the ESP doesn't WDT reset.

      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void sendDataControl()
{
  //read JSON data
  File file_on = SPIFFS.open("/raw_on.json", "r");
  File file_off = SPIFFS.open("/raw_off.json", "r");
  File file_cool = SPIFFS.open("/raw_heat.json", "r");
  File file_heat = SPIFFS.open("/raw_cool.json", "r");

  const size_t size_j = JSON_ARRAY_SIZE(2400);
  DynamicJsonDocument doc1(size_j);
  DynamicJsonDocument doc2(size_j);
  DynamicJsonDocument doc3(size_j);
  DynamicJsonDocument doc4(size_j);

  DeserializationError error1 = deserializeJson(doc1, file_on);
  DeserializationError error2 = deserializeJson(doc2, file_off);
  DeserializationError error3 = deserializeJson(doc3, file_cool);
  DeserializationError error4 = deserializeJson(doc4, file_heat);

  uint16_t raw_on[439];
  uint16_t raw_off[439];
  uint16_t raw_cool[439];
  uint16_t raw_heating[439];

  if (file_on) {
    // Serial.println("============================= ON RAW ==============================");
    for (int x = 0; x < sizeof(raw_on) / sizeof(uint16_t); x = x + 1)
    {
      raw_on[x] = doc1["raw"][x].as<uint16_t>();
      //Serial.print(x);
      //Serial.print(". ");
      //Serial.println(raw_on[x]);
      delay(1);
    }
  }

  if (file_off) {
    //Serial.println("============================= OFF RAW ==============================");
    for (int z = 0; z < sizeof(raw_off) / sizeof(uint16_t); z = z + 1)
    {
      raw_off[z] = doc2["raw"][z].as<uint16_t>();
      //Serial.print(z);
      //Serial.print(". ");
      // Serial.println(raw_off[z]);
      delay(1);
    }
  }

  if (file_cool) {
    // Serial.println("============================= COOL RAW ==============================");
    for (int y = 0; y < sizeof(raw_cool) / sizeof(uint16_t); y = y + 1)
    {
      raw_cool[y] = doc3["raw"][y].as<uint16_t>();
      // Serial.print(y);
      //Serial.print(". ");
      //Serial.println(raw_cool[y]);
      delay(1);
    }
  }
  if (file_heat) {
    // Serial.println("============================= HEATING RAW ==============================");
    for (int a = 0; a < sizeof(raw_heating) / sizeof(uint16_t); a = a + 1)
    {
      raw_heating[a] = doc4["raw"][a].as<uint16_t>();
      //Serial.print(a);
      //Serial.print(". ");
      //Serial.println(raw_heating[a]);
      delay(1);
    }

  }
  if (Button_on == true) {
    Serial.println("AC ON");
    irsend.sendRaw(raw_on, sizeof(raw_on) / sizeof(raw_on[0]), 38);  // Send a raw data capture at 38kHz.
  }

  if (Button_off == true) {
    Serial.println("AC OFF");
    irsend.sendRaw(raw_off, sizeof(raw_off) / sizeof(raw_off[0]), 38);  // Send a raw data capture at 38kHz.
  }
  if (Button_cool == true) {
    Serial.println("AC COOL MODE");
    irsend.sendRaw(raw_cool, sizeof(raw_cool) / sizeof(raw_cool[0]), 38);  // Send a raw data capture at 38kHz.
  }

  if (Button_heat == true) {
    Serial.println("AC HEAT MODE");
    irsend.sendRaw(raw_heating, sizeof(raw_heating) / sizeof(raw_heating[0]), 38);  // Send a raw data capture at 38kHz.

  }
  file_on.close();
  file_off.close();
  file_cool.close();
  file_heat.close();

}

void setup() {

  // put your setup code here, to run once:
  irrecv.enableIRIn();  // Start up the IR receiver.
  irsend.begin();       // Start up the IR sender.

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED2, 1);
  digitalWrite(LED3, 1);

  Serial.begin(115200);               // to enable Serial Commmunication with connected Esp32 board
  delay(500);
  WiFi.mode(WIFI_AP_STA);           // changing ESP9266 wifi mode to AP + STATION

  WiFi.softAP(Apssid, Appassword);         //Starting AccessPoint on given credential
  IPAddress myIP = WiFi.softAPIP();        //IP Address of our Esp32 accesspoint(where we can host webpages, and see data)
  Serial.print("Access Point IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");
  Serial.println("");

  delay(1500);
  Serial.println("connecting to Wifi:");
  Serial.println(Wifi_ssid);

  WiFi.begin(Wifi_ssid, Wifi_password);                  // to tell Esp32 Where to connect and trying to connect
  while (WiFi.status() != WL_CONNECTED) {                // While loop for checking Internet Connected or not
    delay(500);
    digitalWrite(LED1, HIGH);
    delay(500);
    digitalWrite(LED1, LOW);

    if (WiFi.status() == WL_CONNECTED)
    {
      digitalWrite(LED1, 1);
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());            // successful Connection of Esp32,
  // printing Local IP given by your Router or Mobile Hotspot,
  // Esp32 connect at this IP  see in advanced Ip scanner
  Serial.println("");

  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();

  bool success = SPIFFS.begin();

  if (!success) {
    Serial.println("Error mounting the file system");
    return;
  }

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 1); //1 = High, 0 = Low

}

void loop() {
  captureData();
  sendDataControl();
  Blynk.run();
}
