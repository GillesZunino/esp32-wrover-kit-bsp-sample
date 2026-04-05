/* ESP32-WROVER-KIT default example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <bsp/esp_wrover_kit.h>

#include "pretty_effect.h"

/*
 This code displays some fancy graphics on the 320x240 LCD on an ESP-WROVER-KIT board.

 The ILI9341/ST7789V has an C/D line, which is connected to a GPIO. It expects this
 line to be low for a command and high for data
*/


static const char *TAG = "wrover-kit-sample";


// We process lines by PARALLEL_LINES at a time. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

// We limit the framerate by TARGET_FPS
#define TARGET_FPS 30
#define FRAME_PERIOD_TICKS pdMS_TO_TICKS(1000 / TARGET_FPS)


// Type to pass LCD handles to the display task
typedef struct {
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
} display_task_args_t;


// Simple routine to generate some patterns and send them to the LCD. Don't expect anything too impressive
static void animate_esp32_image(esp_lcd_panel_handle_t bsp_lcd_panel_handle, esp_lcd_panel_io_handle_t bsp_lcd_io_handle)
{
    (void)bsp_lcd_io_handle;
    uint16_t *lines[2];
#if CONFIG_LCD_BUFFER_IN_PSRAM
    uint32_t mem_cap = MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA;
    ESP_LOGI(TAG, "Get LCD buffer from PSRAM");
#else
    uint32_t mem_cap = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    ESP_LOGI(TAG, "Get LCD buffer from internal");
#endif

    // Allocate memory for the pixel buffers
    for (int i = 0; i < 2; i++) {
        lines[i] = heap_caps_malloc(BSP_LCD_V_RES * PARALLEL_LINES * sizeof(uint16_t), mem_cap);
        assert(lines[i] != NULL);
    }

    int frame = 0;
    int calc_line = 0;
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        frame++;
        for (int y = 0; y < BSP_LCD_H_RES; y += PARALLEL_LINES) {
            int lines_to_draw = BSP_LCD_H_RES - y;
            if (lines_to_draw > PARALLEL_LINES) {
                lines_to_draw = PARALLEL_LINES;
            }

            // Calculate one horizontal stripe and push it via esp_lcd panel API.
            pretty_effect_calc_lines(lines[calc_line], y, frame, lines_to_draw);
            esp_err_t ret = esp_lcd_panel_draw_bitmap(bsp_lcd_panel_handle, 0, y, BSP_LCD_V_RES, y + lines_to_draw, lines[calc_line]);
            assert(ret == ESP_OK);

            calc_line = (calc_line == 1) ? 0 : 1;
        }

        vTaskDelayUntil(&last_wake, FRAME_PERIOD_TICKS);
    }
}


static void animate_rgb_led(void) {
    bool red = false;
    bool green = true;
    bool blue = false;

    while (1) {
        bsp_led_set(BSP_LED_RED, red);
        red = !red;

        bsp_led_set(BSP_LED_GREEN, green);
        green = !green;

        bsp_led_set(BSP_LED_BLUE, blue);
        blue = !blue;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


static void animate_esp32_image_task(void *pvParameters) {
    display_task_args_t *args = (display_task_args_t *)pvParameters;
    animate_esp32_image(args->panel, args->io);
    vTaskDelete(NULL);
}


static void animate_rgb_led_task(void *pvParameters) {
    (void)pvParameters;
    animate_rgb_led();
    vTaskDelete(NULL);
}


void app_main(void) {
    esp_lcd_panel_handle_t bsp_lcd_panel_handle;
    esp_lcd_panel_io_handle_t bsp_lcd_io_handle;

    bsp_display_config_t bsp_lcd_config = {
        .max_transfer_sz = BSP_LCD_DRAW_BUFF_SIZE * sizeof(uint16_t)
    };

    // Initialize the display and get the handles for the panel and IO, which will be used for drawing
    ESP_ERROR_CHECK(bsp_display_new(&bsp_lcd_config, &bsp_lcd_panel_handle, &bsp_lcd_io_handle));

    // Keep panel orientation non-mirrored for direct draw coordinates
    // This makes the image look right side up when the board is standing with the USB connector facing straight up
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(bsp_lcd_panel_handle, false, false));

    // Use landscape coordinate mapping for this demo's drawing logic (320x240)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(bsp_lcd_panel_handle, true));

    // bsp_display_new() performs reset/init; display output and backlight are enabled explicitly
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(bsp_lcd_panel_handle, true));

    // Sets the brightness of the display - This also turns the display on
    ESP_ERROR_CHECK(bsp_display_brightness_set(50));

    // Initialize the effect displayed
    ESP_ERROR_CHECK(pretty_effect_init());

    // Initialize LED on the board
    ESP_ERROR_CHECK(bsp_leds_init());

    display_task_args_t display_args = {
        .panel = bsp_lcd_panel_handle,
        .io = bsp_lcd_io_handle,
    };

    TaskHandle_t display_task_handle = NULL;
    TaskHandle_t rgb_led_task_handle = NULL;

    ESP_ERROR_CHECK(xTaskCreatePinnedToCore(animate_esp32_image_task, "display_task", 2048, &display_args, tskIDLE_PRIORITY + 1, &display_task_handle, 1) == pdPASS ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(xTaskCreatePinnedToCore(animate_rgb_led_task, "rgb_led_task", 2048, NULL, tskIDLE_PRIORITY + 1, &rgb_led_task_handle, 0) == pdPASS ? ESP_OK : ESP_FAIL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
