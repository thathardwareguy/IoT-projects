#include <SoftwareSerial.h>
SoftwareSerial SIM800(4,3); // RX, TX 
#include <ArduinoJson.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,10,11,12,13);
#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;
String sendToServer="";
String url = "http://iot-databackend-dump.herokuapp.com/details";
int sensorValue = 0,value = 0,push_counter=0;
int status=0; 
unsigned long product_count = 0;
unsigned long pulse_time = 0;  
unsigned long last_pulse_time = 0; 
int m,voltage;// initialise variable m
float n;//initialise variable n
unsigned long startTime;
#define five_mins 120000UL
float temperature=0.00,batt_voltage=0.00,vin = 0.0,vout = 0.015912;
double current;
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 7
#define FONA_RST 6
#define battery_pin A1
#define network_led 5
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SIM800.begin(9600);
  pinMode(FONA_RST,OUTPUT);
  digitalWrite(FONA_RST,LOW);
  pinMode(network_led,OUTPUT);
  lcd.begin (16,2);
  lcd.clear();
  lcd.setCursor(0,0);
  emon1.current(5,60); 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting...");
  delay(15000);
  lcd.clear();
  lcd.print("Connected");
  Serial.println("Connected to cell tower");
  digitalWrite(network_led,HIGH);
  delay(2000);
  digitalWrite(network_led,LOW);
  delay(1000);
  attachInterrupt(0, add, FALLING);  
  product_count = 0;
}

void loop() {
  sensors.requestTemperatures(); 
  // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  temperature = sensors.getTempCByIndex(0);
  m=analogRead(A0);
  n=(m* .330250);
  voltage = n;
  double Irms1 = emon1.calcIrms(1480);  // Calculate Irms only
  current = Irms1-0.11;
  batt_voltage = battery_voltage();
  if (voltage==0)
  {
  current=0.00;  
  }
  
  current = abs(current);
  if(current<=0.09)
  {
  current=0.00;
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("V:"));
  lcd.print(voltage);
  lcd.print(F("V"));
  lcd.setCursor(0,1);
  lcd.print(F("Temp:"));
  lcd.print(temperature); 
  lcd.print((char)223);
  lcd.print("C");
  delay(2000);
  lcd.clear();
  lcd.print(F("I:"));
  lcd.print(current);
  lcd.print(F("A"));
  lcd.setCursor(0,1);
  lcd.print(F("Batt_volt:"));
  lcd.print(batt_voltage);
  lcd.print(F("V"));
  delay(2000);
  lcd.clear();
  lcd.print(F("Count:"));
  lcd.print(product_count);
  delay(2000);
  if (millis() - startTime > five_mins)
  {
    // Put your code that runs every five mins here
    sendToCloud();
    startTime = millis();
    push_counter++;
  }
  if (push_counter>=5)
  {
  digitalWrite(FONA_RST,HIGH);
  delay(2000);
  digitalWrite(FONA_RST,LOW);  
  delay(1000);
  push_counter = 0;  
  }
}

float battery_voltage()
{
   value = analogRead(battery_pin);
   Serial.println(value);
   vin = vout * value; 
   Serial.print(F("INPUT V= "));
   Serial.println(vin,1);
   delay(100); 
   return vin;
}
void sendToCloud()
{
digitalWrite(network_led,HIGH);
lcd.clear();
lcd.print(F("Sending..."));
const int capacity = JSON_OBJECT_SIZE(20);
StaticJsonDocument<capacity> doc;
JsonObject obj1 = doc.to<JsonObject>();
 obj1["voltage"] = voltage;
 obj1["current"] = current;
 obj1["temperature"] = temperature;
 obj1["batteryVoltage"] = batt_voltage;
 obj1["count"] = product_count;
 serializeJsonPretty(doc,sendToServer);
 Serial.print(sendToServer);
 Serial.println(" --- Start GPRS & HTTP --- ");
 gsm_send_serial("AT+CIPSHUT");
 gsm_send_serial("AT+SAPBR=1,1");
 gsm_send_serial("AT+SAPBR=2,1");
 gsm_send_serial("AT+SAPBR=3,1,\"APN\",\"web.gprs.mtnnigeria.net\"");  // APN JTM2M web.gprs.mtnnigeria.net
 gsm_send_serial("AT+HTTPINIT");
 gsm_send_serial("AT+HTTPPARA=CID,1");
 gsm_send_serial("AT+HTTPPARA=URL," + url);
 gsm_send_serial("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
 gsm_send_serial("AT+HTTPDATA=" + String(sendToServer.length()) + ",100000");
 gsm_send_serial(sendToServer);
 gsm_send_serial("AT+HTTPACTION=1");
 //check_server_response();
 gsm_send_serial("AT+HTTPREAD");
 gsm_send_serial("AT+HTTPTERM");
 gsm_send_serial("AT+SAPBR=0,1");
 gsm_send_serial("AT+CIPSHUT");
 sendToServer = "";
 lcd.clear();
 lcd.print(F(">>200:OK"));
 digitalWrite(network_led,LOW);
}
void gsm_send_serial(String command) {
  Serial.println("Send ->: " + command);
  SIM800.println(command);
  long wtimer = millis();
  while (wtimer + 3000 > millis()) {
    while (SIM800.available()) {
      Serial.write(SIM800.read());
    }
  }
  Serial.println();
}
void add()
 {
  pulse_time = millis();
  if (pulse_time - last_pulse_time > 350)
      {
       product_count++; 
       last_pulse_time = pulse_time;
      }
 }
