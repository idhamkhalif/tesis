#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <BlynkSimpleEsp32.h>

#define pin 25

const uint16_t IrPin = 18; // ESP32 GPIO pin to use. Recommended: (D4).

char auth[] = "n1CHCyJXFYAUUe3h0dmH5YF1gcv-OkeI";// ключ от проекта Blynk
char ssid[] = "ccitftui"; // ssid wi-fi сети
char pass[] = "123456789"; // пароль от wi-fi сети

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

IRsend irsend(IrPin);  // Set the GPIO to be used to sending the message.


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
  Serial.begin(115200);

  pinMode(pin, OUTPUT);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
  }
  Blynk.begin(auth, ssid, pass);

  irsend.begin();

#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266

  if (!SPIFFS.begin())
  {
    Serial.println("Error intializing SPIFFS!");
    while (true) {}
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  sendDataControl();
  Blynk.run();
}
