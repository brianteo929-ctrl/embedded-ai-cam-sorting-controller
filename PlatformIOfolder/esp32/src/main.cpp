#include "esp_camera.h" // ESP32 camera driver (interfaces with OV3660 sensor)
#include <WiFi.h> // WiFi library (used for Access Point + networking)

// =====================
// ACCESS POINT SETTINGS
// ESP32 will act as a WiFi router
// =====================
const char* ssid = "ESP32-CAM-MB"; // Network name broadcast by ESP32
const char* password = "BrianESP32"; // Password (must be ≥ 8 chars)

// =====================
// CAMERA PIN MAPPING (AI Thinker ESP32-CAM)
// Maps ESP32 GPIO pins to camera hardware signals
// =====================
#define PWDN_GPIO_NUM 32 // Power-down control for camera module
#define RESET_GPIO_NUM -1 // Reset pin not used on this board
#define XCLK_GPIO_NUM 0 // External clock signal sent to camera
#define SIOD_GPIO_NUM 26 // SCCB (I2C-like) data line for camera config
#define SIOC_GPIO_NUM 27 // SCCB clock line

// Parallel data bus (camera sends pixel data across these pins)
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5

#define VSYNC_GPIO_NUM 25 // Frame synchronization signal
#define HREF_GPIO_NUM 23 // Line synchronization signal
#define PCLK_GPIO_NUM 22 // Pixel clock (timing for data read)

// =====================
// MOTOR PINS (DRV8825)
// =====================
#define STEP_PIN 12
#define DIR_PIN 13
#define ENABLE_PIN 14

// Create a simple HTTP server on port 80
WiFiServer server(80);

// =====================
// CAMERA TASK (FreeRTOS)
// Handles incoming HTTP requests and streams video
// =====================
void cameraTask(void *pvParameters) {

WiFiClient client;

while (true) {

// Check if a client (browser) has connected
client = server.available();
if (!client) {
  vTaskDelay(10); // allow other tasks to run
  continue;
}

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
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  // Stream frames continuously while client is connected
  while (client.connected()) {

    // Capture frame from camera
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) continue;

    // Send frame boundary and headers
    client.print("--frame\r\n");
    client.print("Content-Type: image/jpeg\r\n\r\n");

    // Send JPEG image data
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    // Return frame buffer to driver (IMPORTANT for memory)
    esp_camera_fb_return(fb);

    // Allow RTOS to switch tasks (prevents blocking motor)
    vTaskDelay(1);
  }
}

// Close client connection when done
client.stop();

}
}

// =====================
// MOTOR TASK (FreeRTOS)
// Runs independently of camera
// =====================
void motorTask(void *pvParameters) {

digitalWrite(ENABLE_PIN, LOW); // enable driver

while (true) {

digitalWrite(DIR_PIN, HIGH); // set direction

// Rotate motor (200 steps ≈ 1 revolution)
for (int i = 0; i < 200; i++) {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(800);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(800);
}

// Wait before next movement
vTaskDelay(2000 / portTICK_PERIOD_MS);

}
}

// =====================
// SETUP FUNCTION (runs once at boot)
// Initializes hardware and networking
// =====================
void setup() {

Serial.begin(115200); // Start UART debugging

// =====================
// MOTOR SETUP
// =====================
pinMode(STEP_PIN, OUTPUT);
pinMode(DIR_PIN, OUTPUT);
pinMode(ENABLE_PIN, OUTPUT);

// =====================
// CAMERA CONFIGURATION STRUCT
// =====================
camera_config_t config;

config.ledc_channel = LEDC_CHANNEL_0;
config.ledc_timer = LEDC_TIMER_0;

// Assign camera data pins
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

// Camera configuration bus (I2C)
config.pin_sccb_sda = SIOD_GPIO_NUM;
config.pin_sccb_scl = SIOC_GPIO_NUM;

// Power + reset control
config.pin_pwdn = PWDN_GPIO_NUM;
config.pin_reset = RESET_GPIO_NUM;

// Clock and image format settings
config.xclk_freq_hz = 20000000;
config.pixel_format = PIXFORMAT_JPEG;

// Resolution / quality tradeoff
config.frame_size = FRAMESIZE_QVGA;
config.jpeg_quality = 12;
config.fb_count = 1;

// Initialize camera hardware
if (esp_camera_init(&config) != ESP_OK) {
Serial.println("Camera init failed");
return;
}

// =====================
// START ACCESS POINT
// =====================
WiFi.softAP(ssid, password);
server.begin();

Serial.println("AP Started");
Serial.println(WiFi.softAPIP());

// =====================
// CREATE FREERTOS TASKS
// =====================
xTaskCreate(cameraTask, "Camera Task", 8192, NULL, 1, NULL);
xTaskCreate(motorTask, "Motor Task", 4096, NULL, 1, NULL);
}

// =====================
// LOOP FUNCTION (unused)
// FreeRTOS handles execution
// =====================
void loop() {}