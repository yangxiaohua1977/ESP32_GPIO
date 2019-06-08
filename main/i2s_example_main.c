/* I2S Example

    This example code will output 100Hz sine wave and triangle wave to 2-channel of I2S driver
    Every 5 seconds, it will change bits_per_sample [16, 24, 32] for i2s data

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/xtensa_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include <math.h>
#include "audio.h"
#include "pcm_code.h"
#include "pcm_code_16k.h"

#define SAMPLE_RATE     (16000)
#define I2S_NUM         (0)
#define I2S_BCK_IO      (GPIO_NUM_5)
#define I2S_WS_IO       (GPIO_NUM_25)
#define I2S_DO_IO       (GPIO_NUM_26)
#define I2S_DI_IO       (-1)

#define SAMPLE_PER_CYCLE (SAMPLE_RATE/WAVE_FREQ_HZ)
/*
* playback the voice stored in pcm_example[]
*/
void voice_play(void)
{
    uint8_t *samples_data = malloc(6768 * 16 * 2);
    size_t i2s_bytes_write = 0;
    
    memset(samples_data,0, 6768*16*2);
    memcpy(samples_data, pcm_sample_16k, 6768 * 16);
    i2s_set_clk(I2S_NUM, SAMPLE_RATE, 16, 1);
    i2s_write(I2S_NUM, samples_data, 6768 * 16, &i2s_bytes_write, portMAX_DELAY);
    free(samples_data);
}
/*
* Initialize I2S for 16kHz sample rate and 16bit data length
* set I2S IO port, Enable clock
* 
*/
void i2s_setup(void)
{
    i2s_config_t i2s_config = {
	.mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
	.sample_rate = SAMPLE_RATE,
	.bits_per_sample = 16,
	.channel_format = I2S_CHANNEL_FMT_ALL_LEFT,                           //2-channels
	.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
	.dma_buf_count = 6,
	.dma_buf_len = 60,
	.use_apll = false,
	.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
    };
    i2s_pin_config_t pin_config = {
	.bck_io_num = I2S_BCK_IO,
	.ws_io_num = I2S_WS_IO,
	.data_out_num = I2S_DO_IO,
	.data_in_num = I2S_DI_IO                                               //Not used
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    WRITE_PERI_REG(PIN_CTRL,(CLK_OUT3<<CLK_OUT3_S) | (CLK_OUT2 << CLK_OUT2_S) | (0 << CLK_OUT1_S) );
    PIN_INPUT_DISABLE(PERIPHS_IO_MUX_GPIO0_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    
}
