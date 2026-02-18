#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <INA226_WE.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define RELAY_PIN 26

const char* ssid = "OC LA NET KEKUTHA?";
const char* password = "11111111";

WebServer server(80);

INA226_WE inaBattery = INA226_WE(0x44);
INA226_WE inaSC = INA226_WE(0x45);

float peakBattery = 0;
float peakSC = 0;
float sumSquaredBattery = 0;
unsigned long sampleCount = 0;

String csvData = "Time,Battery(mA),SC(mA),PeakBatt,PeakSC,RMSBatt\n";
unsigned long startTime;

void handleData() {

  float batt = inaBattery.getCurrent_mA();
  float sc = inaSC.getCurrent_mA();

  if (isnan(batt)) batt = 0;
  if (isnan(sc)) sc = 0;

  if (abs(batt) > peakBattery) peakBattery = abs(batt);
  if (abs(sc) > peakSC) peakSC = abs(sc);

  sumSquaredBattery += batt * batt;
  sampleCount++;

  float rmsBatt = sqrt(sumSquaredBattery / sampleCount);

  String json = "{";
  json += "\"batteryCurrent\":" + String(batt,2) + ",";
  json += "\"scCurrent\":" + String(sc,2) + ",";
  json += "\"peakBattery\":" + String(peakBattery,2) + ",";
  json += "\"peakSC\":" + String(peakSC,2) + ",";
  json += "\"rmsBattery\":" + String(rmsBatt,2);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void relayOn(){
  digitalWrite(RELAY_PIN, LOW);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "ON");
}

void relayOff(){
  digitalWrite(RELAY_PIN, HIGH);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OFF");
}

void downloadCSV(){
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/csv", csvData);
}

void setup(){

  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  Wire.begin(SDA_PIN, SCL_PIN);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) delay(500);

  Serial.println(WiFi.localIP());

  inaBattery.init();
  inaSC.init();

  startTime = millis();

  server.on("/data", handleData);
  server.on("/relayOn", relayOn);
  server.on("/relayOff", relayOff);
  server.on("/download", downloadCSV);

  server.begin();
}

void loop(){

  server.handleClient();

  static unsigned long lastLog = 0;

  if(millis() - lastLog > 1000){
    lastLog = millis();

    float batt = inaBattery.getCurrent_mA();
    float sc = inaSC.getCurrent_mA();

    if (isnan(batt)) batt = 0;
    if (isnan(sc)) sc = 0;

    if (abs(batt) > peakBattery) peakBattery = abs(batt);
    if (abs(sc) > peakSC) peakSC = abs(sc);

    sumSquaredBattery += batt * batt;
    sampleCount++;

    float rmsBatt = sqrt(sumSquaredBattery / sampleCount);

    unsigned long t = (millis() - startTime)/1000;

    csvData += String(t) + ",";
    csvData += String(batt,2) + ",";
    csvData += String(sc,2) + ",";
    csvData += String(peakBattery,2) + ",";
    csvData += String(peakSC,2) + ",";
    csvData += String(rmsBatt,2) + "\n";
  }
}
