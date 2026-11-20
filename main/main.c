#include "camera_service.h"
#include "controller.h"
#include "web.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

void app_main(void)
{
    web_prepare_credentials();
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_err_t camera = camera_service_init();
    ESP_LOGI("MAIN", "camera=%s inference=%s position=unknown", esp_err_to_name(camera), inference_backend());
    ESP_ERROR_CHECK(controller_init());
    ESP_ERROR_CHECK(web_start());
    control_status_t status;
    controller_status(&status);
    ESP_LOGI("MAIN", "state=%s armed=%u referenced=%u motors=%s internal_free=%u psram_free=%u",
        fsm_state_name(status.fsm.state), (unsigned)status.fsm.armed, (unsigned)status.fsm.referenced,
#if CONFIG_SORTER_MOTOR_HARDWARE
        "hardware",
#else
        "simulated",
#endif
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
