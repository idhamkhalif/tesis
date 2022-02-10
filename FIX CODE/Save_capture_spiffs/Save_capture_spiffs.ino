#include <ArduinoJson.h>
#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>
#include "FS.h"
#include "SPIFFS.h"

const uint16_t kRecvPin = 23;
const uint16_t kIrLedPin = 18;

const uint32_t kBaudRate = 115200;
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;  // Milli-Seconds
const uint16_t kFrequency = 38000;  // in Hz. e.g. 38kHz.

// ==================== end of TUNEABLE PARAMETERS ====================

// The IR transmitter.
IRsend irsend(kIrLedPin);
// The IR receiver.
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);
// Somewhere to store the captured message.
decode_results results;

int value = 0;

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
  if (Serial.available() > 0) {
    value = Serial.read();
    Serial.println(value);
    if (irrecv.decode(&results)) {  // We have captured something.

      uint16_t *raw_array = resultToRawArray(&results);
      uint16_t length = getCorrectedRawLength(&results);

      const size_t size_j = JSON_ARRAY_SIZE(2400);
      DynamicJsonDocument doc(size_j);

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

      // Check if an IR message has been received.

      if (value == 49) {
        File ff_on = SPIFFS.open("/raw_on.json", FILE_WRITE);
        serializeJson(doc, ff_on);
        ff_on.close();

        readFile(SPIFFS, "/raw_on.json");
        listDir(SPIFFS, "/", 0);

        Serial.println("ON Data Capture");
        Serial.println("Deepsleep Mode");
       // esp_deep_sleep_start();
      }
      if (value == 50) {
        File ff_off = SPIFFS.open("/raw_off.json", FILE_WRITE);
        serializeJson(doc, ff_off);
        ff_off.close();

        readFile(SPIFFS, "/raw_off.json");
        listDir(SPIFFS, "/", 0);

        Serial.println("OFF Data Capture");
        Serial.println("Deepsleep Mode");
       // esp_deep_sleep_start();
      }
      if (value == 51) {
        File ff_cool = SPIFFS.open("/raw_cool.json", FILE_WRITE);
        serializeJson(doc, ff_cool);
        ff_cool.close();

        readFile(SPIFFS, "/raw_cool.json");
        listDir(SPIFFS, "/", 0);

        Serial.println("COOL Data Capture");
        Serial.println("Deepsleep Mode");
      //  esp_deep_sleep_start();
      }
      if (value == 52) {
        File ff_heat = SPIFFS.open("/raw_heat.json", FILE_WRITE);
        serializeJson(doc, ff_heat);
        ff_heat.close();

        readFile(SPIFFS, "/raw_heat.json");
        listDir(SPIFFS, "/", 0);

        Serial.println("HEAT Data Capture");
        Serial.println("Deepsleep Mode");
        esp_deep_sleep_start();
      }
      yield();  // Or delay(milliseconds); This ensures the ESP doesn't WDT reset.
    }
  }
}

void setup() {
  irrecv.enableIRIn();  // Start up the IR receiver.
  irsend.begin();       // Start up the IR sender.

  Serial.begin(kBaudRate, SERIAL_8N1);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();

  Serial.print("DumbIRRepeater is now running and waiting for IR input on Pin ");
  Serial.println(kRecvPin);
  Serial.print("and will retransmit it on Pin ");
  Serial.println(kIrLedPin);

  bool success = SPIFFS.begin();

  if (!success) {
    Serial.println("Error mounting the file system");
    return;
  }
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 1); //1 = High, 0 = Low
  Serial.println("Control Mode");
  Serial.println("********************");
  Serial.println("If setup mode pres :");
  Serial.println("1 for capture data ON");
  Serial.println("2 for capture data OFF");
  Serial.println("3 for capture data COOL");
  Serial.println("4 for capture data HEAT");

}

void loop() {
  captureData();
  Serial.println("Control Mode");
  delay(5000);
}
