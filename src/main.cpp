#include <Arduino.h>
#define MDASH_APP_NAME "Powerwall"
#include <mDash.h>
#include <WiFi.h>
#include "secrets.h"
#include "EspMQTTClient.h"
#include "esp_adc_cal.h"
#include <Battery18650Stats.h>

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
char buff[512];
int vref = 1100;

#define ADC_PINMY 15
Battery18650Stats battery(ADC_PIN);

const int led = 2;
unsigned long previousMillis = 0;
const long interval = 500;
bool ledState = 0;

float xx = 0;
float x2 = 0;

/// MQTT
EspMQTTClient client(
    SECRET_SSID,
    SECRET_PASS,
    SECRET_MQTT_Broker_server, // MQTT Broker server ip
    SECRET_MQTT_USER,          // Can be omitted if not needed
    SECRET_MQTT_PASSWORD,      // Can be omitted if not needed
    SECRET_MQTT_CLIENT_NAME,   // Client name that uniquely identify your device
    SECRET_MQTT_PORT           // The MQTT port, default to 1883. this line can be omitted
);

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

// const int numReadings = 100;

// int readings[numReadings];      // the readings from the analog input
// int readIndex = 0;              // the index of the current reading
// int total = 0;                  // the running total
// int average = 0;                // the average
// int inputPin = 13;

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // client.subscribe("powerwall", [](const String &payload) {
  //   Serial.println(payload);
  // });
  client.publish("powerwall", "//__START__//"); // You can activate the retain flag by setting the third parameter to true
}

//#include "esp_adc_cal.h"

#define AN_Pot1 13
#define FILTER_LEN 100

uint32_t AN_Pot1_Buffer[FILTER_LEN] = {0};
int AN_Pot1_i = 0;
int AN_Pot1_Raw = 0;
int AN_Pot1_Filtered = 0;

uint32_t readADC_Avg(int ADC_Raw)
{
  int i = 0;
  uint32_t Sum = 0;

  AN_Pot1_Buffer[AN_Pot1_i++] = ADC_Raw;
  if (AN_Pot1_i == FILTER_LEN)
  {
    AN_Pot1_i = 0;
  }
  for (i = 0; i < FILTER_LEN; i++)
  {
    Sum += AN_Pot1_Buffer[i];
  }
  return (Sum / FILTER_LEN);
}

// int vref = 1100;
// void showVoltage()
// {

//   static uint64_t timeStamp = 0;
//   if (millis() - timeStamp > 1000)
//   {
//     timeStamp = millis();
//     uint16_t v = analogRead(ADC_PIN);
//     float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
//     String voltage = "Voltage :" + String(battery_voltage) + "V";
//     Serial.println(voltage);

//     tft.fillScreen(TFT_BLACK);
//     tft.setTextColor(TFT_WHITE, TFT_BLACK);
//     tft.setTextSize(1);
//     tft.setCursor(0, 0, 2);
//     tft.println(voltage);

//     // // Set the font colour to be yellow with no background, set to font 7
//     // tft.setTextColor(TFT_YELLOW); tft.setTextFont(2);
//     // tft.println(1234.56);

//     // // Set the font colour to be red with black background, set to font 4
//     // tft.setTextColor(TFT_RED,TFT_BLACK);    tft.setTextFont(4);
//     // tft.println((long)3735928559, HEX); // Should print DEADBEEF

//     // // Set the font colour to be green with black background, set to font 2
//     // tft.setTextColor(TFT_GREEN,TFT_BLACK);
//     // tft.setTextFont(2);
//     // tft.println("Groop");

//     // // Test some print formatting functions
//     // float fnumber = 123.45;
//     //  // Set the font colour to be blue with no background, set to font 2
//     // tft.setTextColor(TFT_BLUE);    tft.setTextFont(2);
//     // tft.print("Float = "); tft.println(fnumber);           // Print floating point number
//     // tft.print("Binary = "); tft.println((int)fnumber, BIN); // Print as integer value in binary
//     // tft.print("Hexadecimal = "); tft.println((int)fnumber, HEX); // Print as integer number in Hexadecimal

//     while (1)
//       yield(); // We must yield() to stop a watchdog timeout.
//   }
// }

void setup()
{

  pinMode(ADC_EN, OUTPUT);
  digitalWrite(ADC_EN, HIGH);
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars); // Check type of calibration value used to characterize ADC
  
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
  {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
  else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
  {
    Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
  }
  else
  {
    Serial.println("Default Vref: 1100mV");
  }

  tft.init();
  tft.setRotation(1);

  // analogSetPinAttenuation(AN_Pot1, ADC_0db);
  Serial.begin(9600);
  // WiFi.begin(SECRET_SSID, SECRET_PASS);
  // mDashBegin(SECRET_DEVICE_PASSWORD);
  pinMode(led, OUTPUT);
  // client.enableDebuggingMessages();                              // Enable debugging messages sent to serial output
  // client.enableLastWillMessage("powerwall", "I am going offline"); // You can activate the retain flag by setting the third parameter to true

  // for (int thisReading = 0; thisReading < numReadings; thisReading++) {
  //   readings[thisReading] = 0;
  // }

  // // initialize all the readings to 0:
  // for (int thisReading = 0; thisReading < numReadings; thisReading++) {
  //   readings[thisReading] = 0;
  // }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

}

void loop()
{

  uint16_t v = analogRead(ADC_PIN);
  float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  String voltage = "Voltage :" + String(battery_voltage) + "V";
  //Serial.println(voltage);

  AN_Pot1_Raw = analogRead(ADC_PINMY);
  AN_Pot1_Filtered = readADC_Avg(AN_Pot1_Raw);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    ledState = not(ledState);
    digitalWrite(led, ledState);

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 2);
    // double f = (3125.0 - 114.0) / 4095.0;
    // float c = 107 - (int)AN_Pot1_Filtered;
    // float n = c / f * -1;

    //loat s0 = (3125.0-114.0)/4095.0;
    float s1 = AN_Pot1_Filtered * 0.7390767712 + 115.6529252;

    //tft.println(s0);
    tft.println(voltage);
    tft.println(s1);
    tft.println(AN_Pot1_Filtered);

    Serial.println(s1);
    Serial.println(AN_Pot1_Filtered);
  }
}