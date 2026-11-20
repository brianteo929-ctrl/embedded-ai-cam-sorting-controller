#include "web.h"
#include "controller.h"
#include "camera_service.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "bootloader_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char token[33];
static char password[17];
static const char *TAG = "WEB";
static esp_err_t unavailable(httpd_req_t *req, const char *message)
{
    httpd_resp_set_status(req, "503 Service Unavailable");
    return httpd_resp_sendstr(req, message);
}
static void random_hex(char *out, unsigned bytes)
{
    static const char hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < bytes; ++i) {
        uint8_t value; esp_fill_random(&value, 1);
        out[i * 2] = hex[value >> 4]; out[i * 2 + 1] = hex[value & 15];
    }
    out[bytes * 2] = 0;
}
void web_prepare_credentials(void)
{
    /* Use the ADC entropy source before the camera/I2S and Wi-Fi start. */
    bootloader_random_enable();
    random_hex(password, 8);
    random_hex(token, 16);
    bootloader_random_disable();
}
static esp_err_t root(httpd_req_t *req)
{
    static const char page[] =
        "<!doctype html><html><meta name='viewport' content='width=device-width'><title>Waste sorter</title>"
        "<style>body{font:16px system-ui;max-width:900px;margin:2em auto;padding:1em;background:#eef3f1;color:#162b25}"
        "button,select{padding:.7em;margin:.3em}pre{white-space:pre-wrap;background:white;padding:1em}img{max-width:100%%}"
        "#stop{background:#a21d26;color:white}</style><h1>Waste sorter</h1>"
        "<p id='mode'></p><p>Arm enables holding torque in hardware mode. After arming, confirm both axes are at the marked zero before Confirm zero. "
        "No automatic homing or position sensor is installed. Stop disables torque and loses the reference.</p>"
        "<button onclick=cmd('arm')>Arm</button><button onclick=cmd('home')>Confirm zero</button>"
        "<select id='scenario'><option>recyclable</option><option>organic</option><option>hazardous</option><option>empty</option>"
        "<option>low</option><option>unknown</option><option>multiple</option><option>conflict</option><option>error</option></select>"
        "<button onclick=trigger()>Capture and sort</button><button id='stop' onclick=cmd('stop')>STOP</button>"
        "<button onclick=cmd('recover')>Clear fault</button><p id='reply'></p><pre id='status'>Loading...</pre>"
        "<button onclick=preview()>Camera preview on/off</button><br><img id='camera' alt='Camera preview'>"
        "<script>let state;async function cmd(c){try{let r=await fetch('/api/command',{method:'POST',headers:{'X-Sorter-Token':'%s'},body:c});"
        "document.getElementById('reply').textContent=await r.text()}catch(e){document.getElementById('reply').textContent=String(e)}}"
        "function trigger(){cmd(state&&state.inference==='mock'?'trigger-'+document.getElementById('scenario').value:'trigger')}"
        "function preview(){let i=document.getElementById('camera');if(i.hasAttribute('src'))i.removeAttribute('src');else i.src='http://'+location.hostname+':81/stream'}"
        "async function poll(){try{let r=await fetch('/api/status');state=await r.json();document.getElementById('status').textContent=JSON.stringify(state,null,2);"
        "document.getElementById('mode').textContent='Inference: '+state.inference+' | Motors: '+state.motors+' | Verification: OPEN LOOP';"
        "document.getElementById('scenario').disabled=state.inference!=='mock'}catch(e){}setTimeout(poll,1000)}poll()</script></html>";
    char body[4096];
    int n = snprintf(body, sizeof(body), page, token);
    if (n < 0 || (size_t)n >= sizeof(body)) return ESP_FAIL;
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-Frame-Options", "DENY");
    return httpd_resp_send(req, body, n);
}
static esp_err_t get_status(httpd_req_t *req)
{
    control_status_t s; controller_status(&s);
    char body[1024];
    int n = snprintf(body, sizeof(body),
        "{\"state\":\"%s\",\"fault\":\"%s\",\"armed\":%s,\"referenced\":%s,\"healthy\":%s,"
        "\"inference\":\"%s\",\"model_version\":\"%s\",\"motors\":\"%s\",\"camera\":\"%s\",\"camera_ready\":%s,"
        "\"cycle\":%lu,\"decision\":\"%s\",\"bin\":\"%s\",\"confidence\":%.3f,\"position_estimate\":[%ld,%ld],"
        "\"vision_busy\":%s,\"command_count\":%lu,\"command_result\":\"%s\",\"capture_ms\":%lu,\"inference_ms\":%lu,"
        "\"internal_free\":%u,\"internal_largest\":%u,\"psram_free\":%u,\"control_stack_free\":%lu,\"vision_stack_free\":%lu}",
        fsm_state_name(s.fsm.state), fsm_fault_name(s.fsm.fault), s.fsm.armed ? "true" : "false",
        s.fsm.referenced ? "true" : "false", s.healthy ? "true" : "false", inference_backend(), inference_model_version(),
#if CONFIG_SORTER_MOTOR_HARDWARE
        "hardware",
#else
        "simulated",
#endif
#if CONFIG_SORTER_CAMERA_MOCK
        "synthetic",
#else
        "jpeg",
#endif
        camera_service_ready() ? "true" : "false", (unsigned long)s.cycle,
        decision_name(s.decision.reason), bin_name(s.decision.bin), (double)s.decision.confidence,
        (long)s.position[0], (long)s.position[1], s.vision_busy ? "true" : "false", (unsigned long)s.command_count,
        s.command_result, (unsigned long)s.capture_ms, (unsigned long)s.inference_ms,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM), (unsigned long)s.control_stack, (unsigned long)s.vision_stack);
    if (n < 0 || (size_t)n >= sizeof(body)) return ESP_FAIL;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, body, n);
}
static esp_err_t command(httpd_req_t *req)
{
    char supplied[sizeof(token)];
    if (httpd_req_get_hdr_value_len(req, "X-Sorter-Token") != 32 ||
        httpd_req_get_hdr_value_str(req, "X-Sorter-Token", supplied, sizeof(supplied)) != ESP_OK || strcmp(supplied, token))
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Reload the control page.");
    char body[32];
    if (req->content_len == 0 || req->content_len >= sizeof(body)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid command length.");
    unsigned received = 0;
    while (received < req->content_len) {
        int n = httpd_req_recv(req, body + received, req->content_len - received);
        if (n <= 0) return httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "Incomplete command.");
        received += (unsigned)n;
    }
    body[received] = 0;
    control_command_t cmd = {.scenario = MOCK_RECYCLABLE};
    if (!strcmp(body, "arm")) cmd.kind = CMD_ARM;
    else if (!strcmp(body, "home")) cmd.kind = CMD_HOME;
    else if (!strcmp(body, "stop")) cmd.kind = CMD_STOP;
    else if (!strcmp(body, "recover")) cmd.kind = CMD_RECOVER;
    else if (!strcmp(body, "trigger")) cmd.kind = CMD_TRIGGER;
#if CONFIG_SORTER_INFERENCE_MOCK
    else if (!strncmp(body, "trigger-", 8)) {
        static const char *cases[] = {"recyclable", "organic", "hazardous", "empty", "low", "unknown", "multiple", "conflict", "error"};
        unsigned i;
        for (i = 0; i < MOCK_COUNT; ++i) if (!strcmp(body + 8, cases[i])) break;
        if (i == MOCK_COUNT) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown scenario.");
        cmd.kind = CMD_TRIGGER; cmd.scenario = (mock_case_t)i;
    }
#endif
    else return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown command.");
    if (controller_submit(cmd) != ESP_OK) return unavailable(req, "Command queue busy.");
    httpd_resp_set_status(req, "202 Accepted");
    return httpd_resp_sendstr(req, "Queued; check command_result and state.");
}
static esp_err_t capture(httpd_req_t *req)
{
    uint8_t *jpeg; size_t length;
    if (camera_jpeg_copy(&jpeg, &length) != ESP_OK) return unavailable(req, "Camera busy or unavailable.");
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t err = httpd_resp_send(req, (const char *)jpeg, length);
    free(jpeg);
    return err;
}
static esp_err_t stream(httpd_req_t *req)
{
    httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    for (;;) {
        uint8_t *jpeg; size_t length;
        if (camera_jpeg_copy(&jpeg, &length) != ESP_OK) return ESP_FAIL;
        char header[100];
        int n = snprintf(header, sizeof(header), "\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)length);
        esp_err_t err = httpd_resp_send_chunk(req, header, n);
        if (err == ESP_OK) err = httpd_resp_send_chunk(req, (const char *)jpeg, length);
        free(jpeg);
        if (err != ESP_OK) return err;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
static esp_err_t stream_redirect(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1:81/stream");
    return httpd_resp_send(req, NULL, 0);
}
static esp_err_t capture_redirect(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1:81/capture");
    return httpd_resp_send(req, NULL, 0);
}
esp_err_t web_start(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) return err;
    if ((err = esp_event_loop_create_default()) != ESP_OK) return err;
    esp_netif_t *netif = esp_netif_create_default_wifi_ap();
    if (!netif) return ESP_ERR_NO_MEM;
    esp_netif_ip_info_t ip = {0};
    ip.ip.addr = ESP_IP4TOADDR(192, 168, 4, 1); ip.gw = ip.ip;
    ip.netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);
    if ((err = esp_netif_dhcps_stop(netif)) != ESP_OK || (err = esp_netif_set_ip_info(netif, &ip)) != ESP_OK ||
        (err = esp_netif_dhcps_start(netif)) != ESP_OK) return err;
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if ((err = esp_wifi_init(&init)) != ESP_OK) return err;
    wifi_config_t config = {.ap = {.ssid = "ESP32-CAM-MB", .ssid_len = 12, .channel = 1,
        .max_connection = 2, .authmode = WIFI_AUTH_WPA2_PSK}};
    if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM)) != ESP_OK || (err = esp_wifi_set_mode(WIFI_MODE_AP)) != ESP_OK) return err;
    if (!password[0]) return ESP_ERR_INVALID_STATE;
    memcpy(config.ap.password, password, sizeof(password));
    if ((err = esp_wifi_set_config(WIFI_IF_AP, &config)) != ESP_OK || (err = esp_wifi_start()) != ESP_OK) return err;
    ESP_LOGI(TAG, "ssid=ESP32-CAM-MB password=%s url=http://192.168.4.1", config.ap.password);
    httpd_handle_t control = NULL, preview = NULL;
    httpd_config_t http = HTTPD_DEFAULT_CONFIG();
    http.stack_size = 8192; http.max_uri_handlers = 6; http.lru_purge_enable = true;
    http.recv_wait_timeout = 2; http.send_wait_timeout = 2; http.task_priority = 2;
    if ((err = httpd_start(&control, &http)) != ESP_OK) return err;
    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = root},
        {.uri = "/api/status", .method = HTTP_GET, .handler = get_status},
        {.uri = "/api/command", .method = HTTP_POST, .handler = command},
        {.uri = "/capture", .method = HTTP_GET, .handler = capture_redirect},
        {.uri = "/stream", .method = HTTP_GET, .handler = stream_redirect},
    };
    for (unsigned i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i)
        if ((err = httpd_register_uri_handler(control, &routes[i])) != ESP_OK) goto failed;
    http.server_port = 81; http.ctrl_port += 1; http.stack_size = 4096; http.max_uri_handlers = 2;
    if ((err = httpd_start(&preview, &http)) != ESP_OK) goto failed;
    httpd_uri_t route = {.uri = "/stream", .method = HTTP_GET, .handler = stream};
    if ((err = httpd_register_uri_handler(preview, &route)) != ESP_OK) goto failed;
    route = (httpd_uri_t){.uri = "/capture", .method = HTTP_GET, .handler = capture};
    if ((err = httpd_register_uri_handler(preview, &route)) != ESP_OK) goto failed;
    ESP_LOGI(TAG, "HTTP ready: controls=:80 images=:81; /capture redirects to image server");
    return ESP_OK;
failed:
    if (preview) httpd_stop(preview);
    if (control) httpd_stop(control);
    return err;
}
