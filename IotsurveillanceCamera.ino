#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>  // Include the I2C library
#include <DNSServer.h>
#include "esp_camera.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#define OLED_SDA 14
#define OLED_SCL 15


const char *ssid = "ESP32-CAM";
const char *password = "";

const char *dnsName = "speedAmmar"; // Custom domain name for the ESP32-CAM

const byte DNS_PORT = 53;
DNSServer dnsServer;

// Create an instance of the server
AsyncWebServer server(80);

#define CAMERA_MODEL_AI_THINKER 
#include "camera_pins.h"
// IR sensor pins
const int sensorPin1 = 12;
const int sensorPin2 = 13;
// Speed calculation variables
volatile unsigned long startTime = 0;
volatile unsigned long endTime = 0;
const float distance = 0.2; // Distance between sensors in meters
bool speedCalculated = false;
float speed = 0;

void IRAM_ATTR detectsMovement1() {
    if (startTime == 0) { // Only update startTime if it's the first detection
        startTime = millis();
    }
}

void IRAM_ATTR detectsMovement2() {
    if (startTime > 0 && !speedCalculated) { // Ensure start time is recorded and speed not yet calculated
        endTime = millis();
        float timeTaken = (endTime - startTime) / 1000.0; // Convert milliseconds to seconds
        speed = distance / timeTaken;
        speedCalculated = true; // Prevent recalculating until reset
    }
}

// Assuming other includes and settings are correctly added


Adafruit_SH1106 display(OLED_SDA, OLED_SCL);


void setup()
{
  Serial.begin(115200);

 display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // Initialize with the I2C addr 0x3C (for 128x64)
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0,0);      // Start at top-left corner
  display.display();

  pinMode(sensorPin1, INPUT);
    pinMode(sensorPin2, INPUT);
    attachInterrupt(digitalPinToInterrupt(sensorPin1), detectsMovement1, FALLING);
    attachInterrupt(digitalPinToInterrupt(sensorPin2), detectsMovement2, FALLING);


camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_HQVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 4;
  config.fb_count = 1;

  


  // Set up ESP32-CAM as an access point
  WiFi.softAP(ssid, password);

  // IP address of the ESP32-CAM in AP mode
  Serial.println("Access Point IP address: " + WiFi.softAPIP().toString());
   // Print the Access Point IP address to the Serial monitor
    Serial.print("Access Point IP address: ");
    Serial.println(WiFi.softAPIP()); // This line prints the IP address

  // Start the DNS server
dnsServer.start(53, "*", WiFi.softAPIP());

Serial.println("Connecting to camera...");
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Failed to initialize the camera");
    while (1);
  }
  Serial.println("Camera connected and initialized");

 

 // Route to handle root request
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><body>";
    html += "<h2>ESP32-CAM Control Page</h2>";
    html += "<p><a href='/capture'>Capture Image</a></p>";
    
    html += "</body></html>";
    request->send(200, "text/html", html);
});

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    // Send the captured image as a response
    request->send_P(200, "image/jpeg", fb->buf, fb->len);
    esp_camera_fb_return(fb);
  } else {
    request->send(500, "text/plain", "Error capturing image");
  }
});

server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><body>";
    html += "<h2>Speed Measurement</h2>";
    html += "<p>Speed: " + String(speed, 2) + " m/s</p>";
    html += "<p><a href='/'>Home</a></p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });


  // Start the server
  server.begin();
   
}


void drawSpeed() {
  display.clearDisplay();
  display.setCursor(0,0); // Start at top-left corner
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.print(F("Speed: "));
  display.print(speed, 2);
  display.println(F(" m/s"));
  display.display();  // Update the display with the new data
}




void loop()
{
 dnsServer.processNextRequest();

   drawSpeed();

  
  delay(1000);   // Add any additional logic or delays as needed
}
