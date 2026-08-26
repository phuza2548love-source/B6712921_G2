// ------------ข้อ1----------------------
// #include <stdio.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <driver/gpio.h>
// #include "sdkconfig.h"
// // main function
// void app_main(void)
// {
//     gpio_num_t LED1 = (gpio_num_t)2;
//     gpio_reset_pin(LED1);
//     gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
//      while (1)
//      {
//         gpio_set_level(LED1, 1);
//         vTaskDelay(600 / portTICK_PERIOD_MS);
//         gpio_set_level(LED1, 0);
//         vTaskDelay(600 / portTICK_PERIOD_MS);
//     }
// }
// ------------ข้อ2----------------------
// #include <stdio.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <driver/gpio.h>
// #include "sdkconfig.h"
// // main function
// void app_main(void)
// {
//     gpio_num_t LED1 = (gpio_num_t)2;
//     gpio_reset_pin(LED1);
//     gpio_set_direction(LED1, GPIO_MODE_INPUT_OUTPUT);
//     while (1)
//     {
//         gpio_set_level(LED1, 1);
//         vTaskDelay(600 / portTICK_PERIOD_MS);
//         gpio_set_level(LED1, 0);
//         vTaskDelay(600 / portTICK_PERIOD_MS);
//     }
// }
// -------------ข้อ3----------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <driver/gpio.h>
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_pm.h"
#include <esp_adc/adc_oneshot.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include <esp_timer.h>

#define ADC_PIN ADC_CHANNEL_7
#define ADC_UNIT ADC_UNIT_1
#define ADC_BITWIDTH ADC_BITWIDTH_10
#define ADC_ATTEN ADC_ATTEN_DB_12

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO 23
#define LEDC_CHANNEL LEDC_CHANNEL_6
#define LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define LEDC_DUTY 2048
#define LEDC_CLK_SRC LEDC_APB_CLK
#define LEDC_FREQUENCY 1820
gpio_num_t LED1 = (gpio_num_t)2;
gpio_num_t LED3 = (gpio_num_t)22;
int adc_value;
adc_oneshot_unit_handle_t adc_handle;
bool suspendmain = false;

void config_led(void)
{
    gpio_reset_pin(LED1);
    gpio_set_direction(LED1, GPIO_MODE_INPUT_OUTPUT);
    gpio_reset_pin(LED3);
    gpio_set_direction(LED3, GPIO_MODE_INPUT_OUTPUT);
}

void config_interrupts()(void)
{
    button_queue = xQueueCreate(10, sizeof(bool));
    gpio_config_t io_conf = {.intr_type = GPIO_INTR_POSEDGE,
                             .mode = GPIO_MODE_INPUT,
                             .pin_bit_mask = (1ULL << BUTTON_GPIO),
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .pull_up_en = GPIO_PULLUP_ENABLE};
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);
    
}
void config_adc()
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PIN, &config));
}
void config_ledc(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_CLK_SRC,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .gpio_num = LEDC_OUTPUT_IO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
void blinkLED(void *pvParameters)
{
    while (1)
    {
        gpio_set_level(LED1, !gpio_get_level(LED1));
        vTaskDelay(600 / portTICK_PERIOD_MS);
    }
}
void ledState(void *pvParameters)
{
    while (1)
    {
        int ledstatus = gpio_get_level(LED1);
        if (ledstatus == 1)
        {
            ESP_LOGI("LED1", "ON");
        }
        if (ledstatus == 0)
        {
            ESP_LOGI("LED1", "OFF");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void getAdcGenPwm(void *pvParameters)
{
    while (1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_value));
        ESP_LOGI("ADC", "Value: %d", adc_value);
        float duty_data = (511 * (adc_value / 1023.0));

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty_data));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
void app_main(void)
{
    config_led();
    config_adc();
    config_ledc();
    config_interrupts();
    xTaskCreatePinnedToCore(blinkLED, "blink led", 512, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ledState, "สถานะled1", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(getAdcGenPwm, "สถานะled1", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(led3Blink, "สถานะled3", 2048, NULL, 1, NULL, 1);
      while (1)    {       
         if (xQueueReceive(button_queue, &suspendmain, portMAX_DELAY))        
         {            
            ESP_LOGI("Button Counter", "Button pressed %lu times.", button_counter);        
        }    
    }
}



