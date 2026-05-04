#include "esp_camera.h"   // ESP32 camera driver (interfaces with OV2640 sensor)
#include <WiFi.h>         // WiFi library (used for Access Point + networking)

// =====================
// ACCESS POINT SETTINGS
// ESP32 will act as a WiFi router
// =====================
const char* ssid = "ESP32-CAM-MB";   // Network name broadcast by ESP32
const char* password = "BrianESP32"; // Password (must be ≥ 8 chars)

// =====================
// CAMERA PIN MAPPING (AI Thinker ESP32-CAM)
// Maps ESP32 GPIO pins to camera hardware signals
// =====================
#define PWDN_GPIO_NUM     32   // Power-down control for camera module
#define RESET_GPIO_NUM    -1   // Reset pin not used on this board
#define XCLK_GPIO_NUM      0   // External clock signal sent to camera
#define SIOD_GPIO_NUM     26   // SCCB (I2C-like) data line for camera config
#define SIOC_GPIO_NUM     27   // SCCB clock line

// Parallel data bus (camera sends pixel data across these pins)
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25   // Frame synchronization signal
#define HREF_GPIO_NUM     23   // Line synchronization signal
#define PCLK_GPIO_NUM     22   // Pixel clock (timing for data read)

// Create a simple HTTP server on port 80
WiFiServer server(80);

// =====================
// SETUP FUNCTION (runs once at boot)
// Initializes hardware and networking
// =====================
void setup() {
  Serial.begin(115200);   // Start UART debugging (115200 baud standard for ESP32)

  // =====================
  // CAMERA CONFIGURATION STRUCT
  // Defines how ESP32 communicates with the camera
  // =====================
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;   // PWM channel used to generate camera clock
  config.ledc_timer = LEDC_TIMER_0;       // Timer used for PWM generation

  // Assign camera data pins to buffer (parallel bus - 8 bit)
  // ESP reads this at once and makes a full JPEG image
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  // Timing + sync signals
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; 
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  // SCCB (camera configuration bus)
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  // Power + reset control
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  // Clock and image format settings
  config.xclk_freq_hz = 20000000;        // 20 MHz clock for camera sensor
  config.pixel_format = PIXFORMAT_JPEG;  // Output compressed JPEG frames

  // Resolution / quality tradeoff
  config.frame_size = FRAMESIZE_QVGA;    // 320x240 (balanced for performance)
  config.jpeg_quality = 12;              // Lower = better quality, larger size
  config.fb_count = 1;                   // Number of frame buffers (low RAM usage)

  // Initialize camera hardware
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed"); // Debug if initialization fails
    return;
  }

  // =====================
  // START ACCESS POINT
  // ESP32 becomes a standalone WiFi network
  // =====================
  WiFi.softAP(ssid, password);

  Serial.println("AP Started");
  Serial.println(WiFi.softAPIP()); // Typically 192.168.4.1

  server.begin();  // Start HTTP server
}

// =====================
// LOOP FUNCTION (runs continuously)
// Handles incoming HTTP requests and streams video
// =====================
void loop() {

  // Check if a client (browser) has connected
  WiFiClient client = server.available();
  if (!client) return;  // No client → exit loop iteration

  // Read HTTP request line
  String req = client.readStringUntil('\r');
  client.flush(); // Clear remaining request data

  // =====================
  // ROOT ENDPOINT ("/")
  // Sends a simple HTML page with embedded stream
  // =====================
  if (req.indexOf("GET / ") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();

    // Minimal webpage with embedded MJPEG stream
    client.println("<html><body>");
    client.println("<h2>ESP32 CAM</h2>");
    client.println("<img src='/stream'>"); // Browser requests /stream endpoint
    client.println("</body></html>");
  }

  // =====================
  // STREAM ENDPOINT ("/stream")
  // Sends continuous MJPEG frames
  // =====================
  else if (req.indexOf("GET /stream") >= 0) {

    client.println("HTTP/1.1 200 OK");

    // Special content type for MJPEG streaming
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println();

    // Stream frames continuously while client is connected
    while (client.connected()) {

      // Capture frame from camera
      camera_fb_t * fb = esp_camera_fb_get();
      if (!fb) continue;  // Skip if capture failed

      // Send frame boundary and headers
      client.print("--frame\r\n");
      client.print("Content-Type: image/jpeg\r\n\r\n");

      // Send JPEG image data
      client.write(fb->buf, fb->len);

      client.print("\r\n");

      // Return frame buffer to driver (IMPORTANT for memory)
      esp_camera_fb_return(fb);
    }
  }

  // Close client connection when done
  client.stop();
}