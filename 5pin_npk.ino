#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

SoftwareSerial mySerial(16, 17);  // RX, TX RO=16, DI=17

int DE = 2;
int RE = 4;
int relay_pin = 5;
float Moisture;

LiquidCrystal_I2C lcd(0x27, 20, 4); // Set the LCD I2C address to 0x27, and define the dimensions (20x4)

const char* ssid = "Sabbir";
const char* password = "19122918";
const char* server = "api.thingspeak.com";
const String apiKey = "SWX48T6RYPT9A8MK";
WiFiClient client;

void setup() {
  Serial.begin(9600);
  mySerial.begin(4800);
  pinMode(DE, OUTPUT);
  pinMode(RE, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  lcd.init();                      // Initialize the LCD
  lcd.backlight();                 // Turn on backlight

  // Connect to WiFi
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void loop() {
  byte queryData[]{ 0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08 };
  byte receivedData[19];
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);

  mySerial.write(queryData, sizeof(queryData));  // send query data to NPK

  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
  delay(1000);

  if (mySerial.available() >= sizeof(receivedData)) {        // Check if there are enough bytes available to read
    mySerial.readBytes(receivedData, sizeof(receivedData));  // Read the received data into the receivedData array
    processReceivedData(receivedData);
  }
}

void processReceivedData(byte receivedData[]) {
  unsigned int soilHumidity = (receivedData[3] << 8) | receivedData[4];
  unsigned int soilTemperature = (receivedData[5] << 8) | receivedData[6];
  unsigned int soilConductivity = (receivedData[7] << 8) | receivedData[8];
  unsigned int soilPH = (receivedData[9] << 8) | receivedData[10];
  unsigned int nitrogen = (receivedData[11] << 8) | receivedData[12];
  unsigned int phosphorus = (receivedData[13] << 8) | receivedData[14];
  unsigned int potassium = (receivedData[15] << 8) | receivedData[16];

  Moisture = (float)soilHumidity / 10.0;

  lcd.clear();  // Clear the LCD display

  lcd.setCursor(0, 0);
  lcd.print("Hum: ");
  lcd.print(Moisture);

  lcd.setCursor(10, 0);
  lcd.print("EC: ");
  lcd.print(soilConductivity);

  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print((float)soilTemperature / 10.0);

  lcd.setCursor(0, 2);
  lcd.print("NPK: ");
  lcd.print(nitrogen);
  lcd.print("/");
  lcd.print(phosphorus);
  lcd.print("/");
  lcd.print(potassium);

  lcd.setCursor(0, 3);
  lcd.print("pH: ");
  lcd.print((float)soilPH / 10.0);

  controlRelay();

  sendToThingSpeak(soilHumidity, soilTemperature, soilConductivity, soilPH, nitrogen, phosphorus, potassium);
}

void controlRelay() {
  if (Moisture > 25) {
    Serial.println("Soil is wet");
    lcd.setCursor(10, 3);
    lcd.print("Wet");
    digitalWrite(relay_pin, HIGH);
  } else {
    Serial.println("Soil is DRY");
    lcd.setCursor(10, 3);
    lcd.print("Dry");
    digitalWrite(relay_pin, LOW);
  }
}

void sendToThingSpeak(unsigned int soilHumidity, unsigned int soilTemperature, unsigned int soilConductivity, unsigned int soilPH, unsigned int nitrogen, unsigned int phosphorus, unsigned int potassium) {
  if (client.connect(server, 80)) {
    String postData = "api_key=" + apiKey;
    postData += "&field1=" + String((float)soilHumidity / 10.0);
    postData += "&field2=" + String((float)soilTemperature / 10.0);
    postData += "&field3=" + String(soilConductivity);
    postData += "&field4=" + String((float)soilPH / 10.0);
    postData += "&field5=" + String(nitrogen);
    postData += "&field6=" + String(phosphorus);
    postData += "&field7=" + String(potassium);

    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();
    client.print(postData);
    Serial.println("Data sent to ThingSpeak");
  } else {
    Serial.println("Connection to ThingSpeak failed");
  }
}
