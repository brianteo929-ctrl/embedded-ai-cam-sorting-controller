#include "camera_service.h"
#include "controller.h"
#include "web.h"
#include "esp_log.h"
#include "nvs_flash.h"

void app_main(void)
{
    web_prepare_credentials();
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_err_t camera = camera_service_init();
    ESP_LOGI("MAIN", "camera=%s inference=%s position=unknown", esp_err_to_name(camera), inference_backend());
    ESP_ERROR_CHECK(controller_init());
    ESP_ERROR_CHECK(web_start());
}
