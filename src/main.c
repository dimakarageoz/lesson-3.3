#include <math.h>
#include <driver/adc.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

const int MAX_ADC_VALUE = 4095; // 12-bit ADC
const int MAX_LED_PWM_DUTY = 8191; // 13-bit duty resolution
const int MAX_DRIVER_PWM_DUTY = 1023; // 10-bit duty resolution
const int VOLTAGE_SENSOR_CHECK_PERIOD = 10; // 10-bit duty resolution

void adc_pwm_task(void *pvParameters) {
    while (1) {
        int adc_value = adc1_get_raw(ADC1_CHANNEL_1);
        
        uint32_t led = (adc_value * MAX_LED_PWM_DUTY) / MAX_ADC_VALUE;
        uint32_t dcDriver = (adc_value * MAX_DRIVER_PWM_DUTY) / MAX_ADC_VALUE;
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, led);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, dcDriver);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        
        // Wait 10ms before next sampling
        vTaskDelay(pdMS_TO_TICKS(VOLTAGE_SENSOR_CHECK_PERIOD));
    }
}

void adcVoltageSensorSetup() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    // GPIO_2, input power 0 - 1.1 V
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_0);

    // Power (+)
    gpio_reset_pin(40);
    gpio_set_direction(40, GPIO_MODE_OUTPUT);
    gpio_set_level(40, 1);
}

void pwmLedOutputSetup() {
    ledc_timer_config_t ledc_timer_35 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 7000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer_35);

    ledc_channel_config_t ledc_channel_35 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = 35,
        .duty = 0
    };
    ledc_channel_config(&ledc_channel_35);
}

void pwmDcDriverOutputSetup() {
    ledc_timer_config_t ledc_timer_37 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer_37);
    
    ledc_channel_config_t ledc_channel_37 = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = 37,
        .duty = 0
    };
    ledc_channel_config(&ledc_channel_37);
}

void app_main(void) {
    adcVoltageSensorSetup();
    pwmLedOutputSetup();
    pwmDcDriverOutputSetup();

    xTaskCreatePinnedToCore(adc_pwm_task, "ADC_PWM_Task", 2048, NULL, 1, NULL, 0);
}