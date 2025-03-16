#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHTesp.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// Pin definitions
#define DHTPIN 4
#define LED_PIN 48
#define NUMPIXELS 1

// WiFi credentials
const char* ssid = "OnePlus 11R 5G";
const char* password = "qwerty123";

// Global variables
AsyncWebServer server(80);
DHTesp dht;
float temperature = 0;
float humidity = 0;
unsigned long lastReadTime = 0;
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

// HTML page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 DHT Sensor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 0px auto; padding: 15px; }
    h1 { color: #0F3376; margin: 50px auto 30px; }
    .sensor { font-size: 24px; font-weight: bold; margin-bottom: 10px; }
  </style>
  <script>
    setInterval(function() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          document.getElementById("temperature").innerHTML = data.temperature + " &deg;C";
          document.getElementById("humidity").innerHTML = data.humidity + " %";
        }
      };
      xhttp.open("GET", "/sensor-data", true);
      xhttp.send();
    }, 5000);
  </script>
</head>
<body>
  <h1>ESP32-S3 DHT Sensor</h1>
  <div class="sensor">Temperature: <span id="temperature">--</span></div>
  <div class="sensor">Humidity: <span id="humidity">--</span></div>
</body>
</html>
)rawliteral";

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

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32-S3 DHT Server Starting...");
  
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
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Route for sensor data as JSON
  server.on("/sensor-data", HTTP_GET, [](AsyncWebServerRequest *request) {
    char jsonResponse[100];
    if (sensorOk) {
      snprintf(jsonResponse, sizeof(jsonResponse), 
               "{\"temperature\":%.1f,\"humidity\":%.1f,\"timestamp\":%lu,\"status\":\"ok\"}", 
               temperature, humidity, millis());
    } else {
      snprintf(jsonResponse, sizeof(jsonResponse), 
               "{\"error\":\"Sensor read failed\",\"status\":\"error\"}");
    }
    request->send(200, "application/json", jsonResponse);
  });

  // Route for LED control
  server.on("/led", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Response will be sent after body is handled
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    if (doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")) {
      redValue = doc["r"];
      greenValue = doc["g"];
      blueValue = doc["b"];
    }
    
    if (doc.containsKey("brightness")) {
      brightness = doc["brightness"];
    }
    
    if (doc.containsKey("effect")) {
      ledEffect = doc["effect"].as<String>();
    }
    
    // Apply changes immediately for solid effect
    if (ledEffect == "solid") {
      setLedColor(redValue, greenValue, blueValue);
    }
    
    // Send response
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // Route to get current LED status
  server.on("/led-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    char jsonResponse[200];
    snprintf(jsonResponse, sizeof(jsonResponse), 
             "{\"r\":%d,\"g\":%d,\"b\":%d,\"brightness\":%d,\"effect\":\"%s\"}", 
             redValue, greenValue, blueValue, brightness, ledEffect.c_str());
    request->send(200, "application/json", jsonResponse);
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
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