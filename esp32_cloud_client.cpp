#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <DHTesp.h>

// Pin definitions
#define DHTPIN 4
#define LED_PIN 48
#define NUMPIXELS 1

// WiFi credentials
const char* ssid = "OnePlus 11R 5G";
const char* password = "qwerty123";

// Server details
const char* serverUrl = "https://ewwgkwskw4skwk0s8owoggws.cscclub.space"; // Change to your server URL
const char* apiKey = "123456789"; // Must match DEVICE_API_KEY on server

// Global variables
DHTesp dht;
float temperature = 0;
float humidity = 0;
unsigned long lastReadTime = 0;
unsigned long lastServerUpdateTime = 0;
unsigned long lastCommandCheckTime = 0;
bool sensorOk = false;

// Initialize NeoPixel
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// LED variables
int redValue = 0;
int greenValue = 150;
int blueValue = 0;
int brightness = 128;
String ledEffect = "solid";
unsigned long lastLedUpdate = 0;
int breatheValue = 0;
bool breatheDirection = true;
int blinkState = 0;
unsigned long rainbowHue = 0;

// Function to set RGB LED color
void setLedColor(int r, int g, int b) {
  // Apply brightness
  r = map(r, 0, 255, 0, brightness);
  g = map(g, 0, 255, 0, brightness);
  b = map(b, 0, 255, 0, brightness);
  
  // Set NeoPixel color
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
  
  // Store the original values
  redValue = r;
  greenValue = g;
  blueValue = b;
}

// Convert HSV to RGB
void hsvToRgb(unsigned long h, int s, int v, int& r, int& g, int& b) {
  h %= 360;
  float hf = h / 60.0;
  int i = floor(hf);
  float f = hf - i;
  float p = v * (1 - s / 100.0);
  float q = v * (1 - s / 100.0 * f);
  float t = v * (1 - s / 100.0 * (1 - f));

  switch (i) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}

// Update LED based on current effect
void updateLedEffect() {
  if (ledEffect == "solid") {
    setLedColor(redValue, greenValue, blueValue);
  } 
  else if (ledEffect == "blink") {
    unsigned long currentMillis = millis();
    if (currentMillis - lastLedUpdate > 500) {
      lastLedUpdate = currentMillis;
      blinkState = !blinkState;
      
      if (blinkState) {
        setLedColor(redValue, greenValue, blueValue);
      } else {
        setLedColor(0, 0, 0);
      }
    }
  } 
  else if (ledEffect == "breathe") {
    unsigned long currentMillis = millis();
    if (currentMillis - lastLedUpdate > 30) {
      lastLedUpdate = currentMillis;
      
      if (breatheDirection) {
        breatheValue += 5;
        if (breatheValue >= 255) {
          breatheValue = 255;
          breatheDirection = false;
        }
      } else {
        breatheValue -= 5;
        if (breatheValue <= 0) {
          breatheValue = 0;
          breatheDirection = true;
        }
      }
      
      int tempBrightness = brightness;
      brightness = map(breatheValue, 0, 255, 0, tempBrightness);
      setLedColor(redValue, greenValue, blueValue);
      brightness = tempBrightness;
    }
  } 
  else if (ledEffect == "rainbow") {
    unsigned long currentMillis = millis();
    if (currentMillis - lastLedUpdate > 30) {
      lastLedUpdate = currentMillis;
      
      rainbowHue = (rainbowHue + 1) % 360;
      int r, g, b;
      hsvToRgb(rainbowHue, 100, 100, r, g, b);
      setLedColor(r, g, b);
    }
  }
}

void readSensor() {
  // Read humidity and temperature
  TempAndHumidity data = dht.getTempAndHumidity();
  
  // Check if reading was successful
  if (!isnan(data.humidity) && !isnan(data.temperature)) {
    humidity = data.humidity;
    temperature = data.temperature;
    sensorOk = true;
    Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
  } else {
    Serial.println("Failed to read from DHT sensor");
    sensorOk = false;
  }
}

// Send sensor data to server
void sendDataToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot send data.");
    return;
  }
  
  HTTPClient http;
  http.begin(String(serverUrl) + "/api/device/data");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", apiKey);
  
  // Create JSON document
  DynamicJsonDocument doc(1024);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = millis();
  doc["status"] = sensorOk ? "ok" : "error";
  
  // Add LED state
  doc["led"]["r"] = redValue;
  doc["led"]["g"] = greenValue;
  doc["led"]["b"] = blueValue;
  doc["led"]["brightness"] = brightness;
  doc["led"]["effect"] = ledEffect;
  
  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send request
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Server response: " + response);
    
    // Parse response for any pending commands
    DynamicJsonDocument responseDoc(1024);
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error && responseDoc.containsKey("pendingCommands")) {
      JsonArray commands = responseDoc["pendingCommands"].as<JsonArray>();
      
      for (JsonVariant command : commands) {
        String type = command["type"];
        
        if (type == "led") {
          JsonObject data = command["data"];
          
          if (data.containsKey("r") && data.containsKey("g") && data.containsKey("b")) {
            redValue = data["r"];
            greenValue = data["g"];
            blueValue = data["b"];
          }
          
          if (data.containsKey("brightness")) {
            brightness = data["brightness"];
          }
          
          if (data.containsKey("effect")) {
            ledEffect = data["effect"].as<String>();
          }
          
          // Apply changes immediately for solid effect
          if (ledEffect == "solid") {
            setLedColor(redValue, greenValue, blueValue);
          }
          
          Serial.println("Applied LED command from server");
        }
      }
    }
  } else {
    Serial.print("Error sending data. HTTP response code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

// Check for pending commands
void checkForCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot check for commands.");
    return;
  }
  
  HTTPClient http;
  http.begin(String(serverUrl) + "/api/device/commands");
  http.addHeader("X-API-Key", apiKey);
  
  // Send request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Command check response: " + response);
    
    // Parse response for any pending commands
    DynamicJsonDocument responseDoc(1024);
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error && responseDoc.containsKey("pendingCommands")) {
      JsonArray commands = responseDoc["pendingCommands"].as<JsonArray>();
      
      for (JsonVariant command : commands) {
        String type = command["type"];
        
        if (type == "led") {
          JsonObject data = command["data"];
          
          if (data.containsKey("r") && data.containsKey("g") && data.containsKey("b")) {
            redValue = data["r"];
            greenValue = data["g"];
            blueValue = data["b"];
          }
          
          if (data.containsKey("brightness")) {
            brightness = data["brightness"];
          }
          
          if (data.containsKey("effect")) {
            ledEffect = data["effect"].as<String>();
          }
          
          // Apply changes immediately for solid effect
          if (ledEffect == "solid") {
            setLedColor(redValue, greenValue, blueValue);
          }
          
          Serial.println("Applied LED command from server");
        }
      }
    }
  } else {
    Serial.print("Error checking commands. HTTP response code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32-S3 Cloud Client Starting...");
  
  // Initialize NeoPixel
  pixels.begin();
  pixels.clear();
  
  // Set initial LED color (green)
  setLedColor(0, 150, 0);
  
  // Initialize DHT sensor
  Serial.println("Initializing DHT sensor...");
  dht.setup(DHTPIN, DHTesp::DHT11);
  delay(2000);
  
  // Initial sensor reading
  readSensor();
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed! Restarting...");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Send initial data to server
  sendDataToServer();
}

void loop() {
  // Read sensor data every 2 seconds
  unsigned long currentTime = millis();
  if (currentTime - lastReadTime > 2000) {
    readSensor();
    lastReadTime = currentTime;
  }
  
  // Update LED effect
  updateLedEffect();
  
  // Send data to server every 30 seconds
  if (currentTime - lastServerUpdateTime > 30000) {
    sendDataToServer();
    lastServerUpdateTime = currentTime;
  }
  
  // Check for commands every 5 seconds
  if (currentTime - lastCommandCheckTime > 5000) {
    checkForCommands();
    lastCommandCheckTime = currentTime;
  }
  
  // Check WiFi connection every 30 seconds
  static unsigned long lastWifiCheck = 0;
  if (currentTime - lastWifiCheck > 30000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected! Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
    lastWifiCheck = currentTime;
  }
  
  // Give other tasks time to run
  delay(10);
} 