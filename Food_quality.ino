#define BLYNK_TEMPLATE_ID " "
#define BLYNK_TEMPLATE_NAME " "
#define BLYNK_AUTH_TOKEN " "

#include <LiquidCrystal.h>
#include "DHT.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>

#define LIGHT_SENSOR_PIN 13
#define DHT_SENSOR_PIN 4
#define DHT_SENSOR_TYPE DHT11
#define GAS_SENSOR_PIN 35
#define PHpin 34
#define WIFI_SSID " "
#define WIFI_PASSWORD " "


DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);

char blynkAuthToken[] = BLYNK_AUTH_TOKEN;
char wifiSsid[] = WIFI_SSID;
char wifiPassword[] = WIFI_PASSWORD;

BlynkTimer blynkTimer;

int gasPercentage = 0;
int temperatureCelsius = 0;
int humidityPercentage = 0;
float calibration_value = 20.24 - 3.20;  // Calibration offset
int buffer_arr[10]; 
int temp;
float ph_act;

void setup() {
  Serial.begin(115200);
  WiFi.begin(wifiSsid, wifiPassword);
  Serial.print("Connecting to Wi-Fi");
  int wifiAttemptCount = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttemptCount < 20) {
    delay(1000);
    Serial.print(".");
    wifiAttemptCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");
    Serial.println("Connecting to Blynk...");
    Blynk.begin(blynkAuthToken, wifiSsid, wifiPassword);
  } else {
    Serial.println("\nWi-Fi Connection Failed!");
  }

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to");
  lcd.setCursor(0, 1);
  lcd.print("FOOD MONITORING");
  delay(2000);
  lcd.clear();

  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(PHpin, INPUT);

  dht.begin();
  blynkTimer.setInterval(5000L, sendToBlynk);
}

void loop() {
  Blynk.run();
  blynkTimer.run();
  readLightSensor();
  readGasSensor();
  phsensor();
  readTemperatureHumidity();
}

void readLightSensor() {
  int lightState = digitalRead(LIGHT_SENSOR_PIN);
  Serial.print("Light state: ");
  Serial.println(lightState);

  lcd.clear();
  if (lightState == HIGH) {
    Serial.println("It is dark");
    lcd.setCursor(0, 0);
    lcd.print("MINIMAL");
    lcd.setCursor(0, 1);
    lcd.print("LIGHT");
  } else {
    Serial.println("It is light");
    lcd.setCursor(0, 0);
    lcd.print("FOOD EXPOSED");
    lcd.setCursor(0, 1);
    lcd.print("TO LIGHT");
  }
  delay(3000);
}

void readGasSensor() {
  int gasSensorValue = analogRead(GAS_SENSOR_PIN);
  gasPercentage = map(gasSensorValue, 0, 4095, 0, 100);

  Serial.print("ADC VALUE: ");
  Serial.println(gasSensorValue);
  Serial.print("PPM IN % : ");
  Serial.println(gasPercentage);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gas value: ");
  lcd.print(gasPercentage);
  lcd.print("%");
  delay(3000);

  if (gasPercentage < 10) {  // **Food is Normal**
    Serial.println("Food condition: Normal");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FOOD IS NORMAL");
    lcd.setCursor(0, 1);
    lcd.print("SAFE TO STORE");
    delay(3000);
  } else if (gasPercentage >= 10 && gasPercentage < 14) {
    Serial.println("Warning: Food starts to spoil.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FOOD STARTS");
    lcd.setCursor(0, 1);
    lcd.print("TO SPOIL");
    delay(3000);
    //Blynk.logEvent("gas_warning", "Food may start spoiling. Monitor closely.");
  } else if (gasPercentage > 20) {
    Serial.println("ALERT! FOOD IS SPOILED!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ALERT!");
    lcd.setCursor(0, 1);
    lcd.print("FOOD SPOILED");
    delay(3000);
    Blynk.logEvent("gas_alert", "CRITICAL: FOOD SPOILAGE DETECTED!");
  }
}

void readTemperatureHumidity() {
  humidityPercentage = dht.readHumidity();
  temperatureCelsius = dht.readTemperature();

  if (isnan(humidityPercentage) || isnan(temperatureCelsius)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    lcd.clear();
    lcd.print("DHT Read Err");
    delay(2000);
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(humidityPercentage);
  Serial.print(F("% Temp: "));
  Serial.print(temperatureCelsius);
  Serial.println(F("°C"));

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Temp: "));
  lcd.print(temperatureCelsius);
  lcd.print((char)223);  // Print the degree symbol
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print(F("Hum: "));
  lcd.print(humidityPercentage);
  lcd.print(F("%"));
  delay(3000);

  if (humidityPercentage >= 79) {
    Serial.println("FOOD SPOILING DUE TO HIGH HUMIDITY");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FOOD SPOILING");
    lcd.setCursor(0, 1);
    lcd.print("DUE TO HUMIDITY");
    delay(3000);
  }
  else if (temperatureCelsius >= 32) {
    Serial.println("HIGH TEMPERATURE");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HIGH");
    lcd.setCursor(0, 1);
    lcd.print("TEMPERATURE");
    delay(3000);
  }
}
void phsensor() {
    // Read pH sensor values into the buffer
    for (int i = 0; i < 10; i++) { 
        buffer_arr[i] = analogRead(PHpin);
        delay(30);
    }

    // Sort the buffer array (Bubble Sort)
    for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 10; j++) {
            if (buffer_arr[i] > buffer_arr[j]) {
                temp = buffer_arr[i];
                buffer_arr[i] = buffer_arr[j];
                buffer_arr[j] = temp;
            }
        }
    }

    // Take the median 6 values for averaging
    unsigned long int avgval = 0;
    for (int i = 2; i < 8; i++) {
        avgval += buffer_arr[i];
    }

    // Convert ADC value to voltage
    float volt = (float)avgval * 3.3 / 4096.0 / 6;  

    // Convert voltage to pH value using calibration
    ph_act = -5.70 * volt + calibration_value;

    // Print the pH value
    Serial.print("pH Value: ");
    Serial.println(ph_act);

  // Classify the solution
  String solutionType;
  if (ph_act >= 6.30 && ph_act <6) {
    solutionType = "Alkaline";
  } else if (ph_act < 6.0) {
    solutionType = "Acidic";
  } else {
    solutionType = "Neutral";
  }

  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("pH Value: "));
  lcd.print(ph_act);

  lcd.setCursor(0, 1);
  lcd.print(F("Type: "));
  lcd.print(solutionType);

  delay(3000);
}


void sendToBlynk() {
  Serial.print("Gas: ");
  Serial.print(gasPercentage);
  Serial.print(" | Temp: ");
  Serial.print(temperatureCelsius);
  Serial.print(" | Humidity: ");
  Serial.println(humidityPercentage);
  Serial.print(" | pH: ");
  Serial.println(ph_act);

  Blynk.virtualWrite(V0, gasPercentage);
  Blynk.virtualWrite(V1, temperatureCelsius);
  Blynk.virtualWrite(V2, humidityPercentage);
  Blynk.virtualWrite(V3, ph_act);
  Serial.println("Sending to Blynk");
  //if (gasPercentage > 12) {
  //Blynk.logEvent("gas_alert", "FOOD SPOLIAGE DETECTED!");
}
