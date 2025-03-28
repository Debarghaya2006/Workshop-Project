#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "AdafruitIO_WiFi.h"

// Wi-Fi credentials
const char* ssid = "test1";
const char* password = "test1234";

// Adafruit IO credentials
#define IO_USERNAME    "DebarghayaMitra"
#define IO_KEY         "aio_KlDw74b4o0hUfYg7bPX7bvTpprj5"

// DHT22 Sensor settings
#define DHTPIN          4         // Pin connected to the DHT22 sensor
#define DHTTYPE         DHT11     // DHT 22 (AM2302)

// Open-Meteo API endpoint for Kolkata
const char* weatherApiUrl = "https://api.open-meteo.com/v1/forecast?latitude=22.5726&longitude=88.3639&current=temperature_2m,relative_humidity_2m";

// Create DHT object
DHT dht(DHTPIN, DHTTYPE);

// Create Adafruit IO instance
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, ssid, password);

// Create Adafruit IO feeds
AdafruitIO_Feed *temperatureFeed = io.feed("temperature");
AdafruitIO_Feed *humidityFeed = io.feed("humidity");

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi");

  // Connect to Adafruit IO
  io.connect();
  while (io.status() < AIO_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Adafruit IO");

  // Initialize DHT sensor
  dht.begin();
}

void loop() {
  // Read temperature and humidity from DHT22 sensor
  float temperature = dht.readTemperature(); // Temperature in Celsius
  float humidity = dht.readHumidity(); // Humidity in percentage

  // Check if readings are valid
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor! Fetching weather data...");

    // Fetch weather data from Open-Meteo API
    if (!fetchWeatherData(&temperature, &humidity)) {
      Serial.println("Failed to fetch weather data.");
      return;
    }
  }

  // Print the temperature and humidity to the Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C  ");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Send data to Adafruit IO
  temperatureFeed->save(temperature);  // Send temperature data
  humidityFeed->save(humidity);        // Send humidity data

  // Wait for 10 seconds before sending new data
  delay(10000);
}

bool fetchWeatherData(float* temperature, float* humidity) {
  WiFiClientSecure client;
  client.setInsecure(); // Disable SSL certificate verification (use with caution)

  HTTPClient http;
  http.begin(client, weatherApiUrl);

  int httpResponseCode = http.GET();
  if (httpResponseCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    *temperature = doc["current"]["temperature_2m"];
    *humidity = doc["current"]["relative_humidity_2m"];

    http.end();
    return true;
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return false;
}
