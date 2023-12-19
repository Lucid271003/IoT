#define BLYNK_TEMPLATE_ID "TMPL6IOs2nTcs"
#define BLYNK_TEMPLATE_NAME "Project"
#define BLYNK_AUTH_TOKEN "TR9FrIhBsVspemp8_0JtmB5n5-D90O7b"

// Blynk and WifiManager
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>

// Firebase
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define API_KEY "AIzaSyDgFtVxGlrK_i6Ate1QCwc3-xjGqS63kxU"
#define DATABASE_URL "https://database-76c8a-default-rtdb.asia-southeast1.firebasedatabase.app/"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

FirebaseData fbdo;
FirebaseAuth fauth;
FirebaseConfig config;
FirebaseJson json;

char auth[] = BLYNK_AUTH_TOKEN;
WiFiManager wifiMn;
bool signupOK = false;
int buzzer = D1; // D1
int mucCanhbao;
BlynkTimer timer;
int timerID1, timerID2;
int mq2_value;
int button = 0; // D3
boolean buttonState = HIGH;
boolean runMode = 1; // Bật/tắt chế độ cảnh báo
boolean canhbaoState = 0;
WidgetLED led(V0);

void configModeCallback(WiFiManager *myWiFiManager){
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  Serial.begin(115200);
  delay(100);
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  pinMode(button, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW); // Tắt buzzer
  timerID1 = timer.setInterval(4000L, handleTimerID1);

  wifiMn.resetSettings();
  if (!wifiMn.autoConnect("Ho Minh")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  }
  else
  {
    Serial.println("Connected to Wi-Fi");
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &fauth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &fauth);
  Firebase.reconnectWiFi(true);

  // Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();
}

String sensorPath = "/sensor";
String databasePath = "/Asm";
String parentPath;

void loop() {
  Blynk.run();
  timer.run();
  if (digitalRead(button) == LOW) {
    if (buttonState == HIGH) {
      buttonState = LOW;
      runMode = !runMode;
      Serial.println("Run mode: " + String(runMode));
      Blynk.virtualWrite(V4, runMode);
      delay(200);
    }
  } else {
    buttonState = HIGH;
  }
}

String getDatetime() {
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    return String(ptm->tm_mday) + "_" + String(ptm->tm_year + 1900) + " " + timeClient.getFormattedTime();
}

void handleTimerID1() {
  mq2_value = analogRead(A0);
  Blynk.virtualWrite(V1,mq2_value);
  if(led.getValue()) {
    led.off();
  } else {
    led.on();
  }
  if(runMode==1){
    if(mq2_value>mucCanhbao){
      if(canhbaoState==0){
        canhbaoState=1;
        timerID2 = timer.setTimeout(1000L,handleTimerID2);
      }
      digitalWrite(buzzer,HIGH);
      Blynk.virtualWrite(V3,HIGH);
      Serial.println("Đã bật cảnh báo!");
    }else{
      digitalWrite(buzzer,LOW);
      Blynk.virtualWrite(V3,LOW);
      Serial.println("Đã tắt cảnh báo!");
    }
  }else{
    canhbaoState=0;
    digitalWrite(buzzer,LOW);
    Blynk.virtualWrite(V3,LOW);
    Serial.println("Đã tắt cảnh báo!");
  }
  if (Firebase.ready() && signupOK) {
    String datetime = getDatetime();
    Serial.print("time: ");
    parentPath = databasePath + "/" + datetime;

    // set the JSON strings for sensor
    json.set( sensorPath.c_str(), mq2_value);

    // send data to the real-time database
    if (Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void handleTimerID2() {
  canhbaoState=0;
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2, V4);
}

BLYNK_WRITE(V2) {
  mucCanhbao = param.asInt();
}

BLYNK_WRITE(V4) {
  runMode = param.asInt();
}