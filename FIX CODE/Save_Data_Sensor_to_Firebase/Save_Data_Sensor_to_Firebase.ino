#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <FunctionalInterrupt.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define LDR 12
#define SDA1 21
#define SCL1 22
#define PIR  19
#define SNDD 32

#define LED1 25

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String formattedDate;
String dayStamp;
String timeStamp;

Adafruit_BME280 bme;
TwoWire I2Cone = TwoWire(1);
uint32_t n = 400000;

//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "ccitftui"
#define WIFI_PASSWORD "123456789"

/* 2. Define the API Key */
#define API_KEY "AIzaSyDgxGDnxpJoL16m-Po65YmBrQHoYnStU98"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "automationdb-4d0d0"

/* 4. Define the RTDB URL */
#define DATABASE_URL "https://automationdb-4d0d0-default-rtdb.firebaseio.com/"

/* 5. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "idham@gmail.com"
#define USER_PASSWORD "123456789"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;

//The Firestore payload upload callback function
void fcsUploadCallback(CFS_UploadStatusInfo info)
{
  if (info.status == fb_esp_cfs_upload_status_init)
  {
    Serial.printf("\nUploading data (%d)...\n", info.size);
  }
  else if (info.status == fb_esp_cfs_upload_status_upload)
  {
    Serial.printf("Uploaded %d%s\n", (int)info.progress, "%");
  }
  else if (info.status == fb_esp_cfs_upload_status_complete)
  {
    Serial.println("Upload completed ");
  }
  else if (info.status == fb_esp_cfs_upload_status_process_response)
  {
    Serial.print("Processing the response... ");
  }
  else if (info.status == fb_esp_cfs_upload_status_error)
  {
    Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(LDR, INPUT_PULLUP);
  pinMode(PIR, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);

  Serial.println("init I2C");
  I2Cone.begin(SDA1, SCL1, n); // SDA pin 21, SCL pin 22
  delay(100);
  bool status = bme.begin(0x76, &I2Cone);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    digitalWrite(LED1, HIGH);
    delay(500);
    digitalWrite(LED1, LOW);

    if (WiFi.status() == WL_CONNECTED)
    {
      digitalWrite(LED1, 1);
    }
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  timeClient.begin();
  timeClient.setTimeOffset(25200);

  Firebase.setDoubleDigits(2);
}

void loop()
{
  int id = random(0, 99999);
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  if (Firebase.ready() && (millis() - dataMillis > 60000 || dataMillis == 0))
  {
    dataMillis = millis();
    int analog_LDR = analogRead(LDR);
    boolean statusPIR = digitalRead(PIR);
    boolean statusLDR = digitalRead(LDR);
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    double voice = analogRead(SNDD);

    formattedDate = timeClient.getFormattedDate();

    // Extract date
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

    //For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
    FirebaseJson content;
    String documentPath = "a0/id" + String(id);

    //Firestore DB
    Serial.println("Send to Firestore DB");
    content.set("fields/temperature/doubleValue", temp);
    content.set("fields/humidity/doubleValue", hum);
    content.set("fields/voice/doubleValue", voice);
    content.set("fields/PIR/booleanValue", statusPIR);
    content.set("fields/LDR/booleanValue", statusLDR);
    content.set("fields/time/stringValue", timeStamp);
    content.set("fields/date/stringValue", dayStamp);
    content.set("fields/datetime/stringValue", formattedDate);
    Serial.println("Create a document... ");

    //RTDB
    Serial.println("Send to RTDB... ");
    Firebase.RTDB.setDouble(&fbdo, "/sensor/temperature", temp);
    Firebase.RTDB.setDouble(&fbdo, "/sensor/humidity", hum);
    Firebase.RTDB.setDouble(&fbdo, "/sensor/voice", voice);
    Firebase.RTDB.setString(&fbdo, "/sensor/date", dayStamp);
    Firebase.RTDB.setString(&fbdo, "/sensor/time", timeStamp);
    Firebase.RTDB.setString(&fbdo, "/sensor/datetime", formattedDate);
    Firebase.RTDB.setBool(&fbdo, "/sensor/pir", statusPIR);
    Firebase.RTDB.setBool(&fbdo, "/sensor/ldr", statusLDR);
    Serial.print("ok message: " + String(fbdo.errorReason()));

    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw()))
      Serial.printf(" ok\n%s\n\n", fbdo.payload().c_str());
    else
      Serial.println(fbdo.errorReason());

  }
}
