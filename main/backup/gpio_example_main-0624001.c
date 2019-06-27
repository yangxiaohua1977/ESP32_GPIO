/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "driver/spi_master.h"

/**
 * Brief:
 * Detect the evevator up/dn arrival, then, turn on and flash the LED lights, play the voice
 *
 */

#define LED_NUM	    32

#define ZERO_CODE_H   7 
#define ZERO_CODE_L   43

#define ONE_CODE_H    24
#define ONE_CODE_L    26

#define GPIO_DT1_UP  34
#define GPIO_DT1_DN  35

#define GPIO_DT2_UP    32
#define GPIO_DT2_DN    33

#define GPIO_LED_UP    22
#define GPIO_LED_DN    21

#define GPIO_PA_EN		 23

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_LED_UP) | (1ULL<<GPIO_LED_DN) | (1ULL<<GPIO_PA_EN) | (1ULL<<0))

#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_DT1_UP) | (1ULL<<GPIO_DT1_DN)|(1ULL<<GPIO_DT2_UP) | (1ULL<<GPIO_DT2_DN))
#define ESP_INTR_FLAG_DEFAULT 0


#define PIN_NUM_MISO 15
#define PIN_NUM_MOSI 21
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   18

spi_device_handle_t spi;

extern void voice_play(void);
extern void i2s_setup(void);

SemaphoreHandle_t xElevator_UpArrival_Semaphore = NULL;
SemaphoreHandle_t xElevator_DnArrival_Semaphore = NULL;
SemaphoreHandle_t xLED_Flash_Semaphore = NULL;
SemaphoreHandle_t xVoicePlay_Semaphore = NULL;

	
/*
* Brief:
* 	when semaphore is activity, Play the voice via I2S
*/
#if 1
static void task_voice_play(void* arg)
{
	for(;;)
	{
		if(xSemaphoreTake(xVoicePlay_Semaphore, 0)) 
		{
			gpio_set_level(GPIO_PA_EN, 1);
			voice_play();
			gpio_set_level(GPIO_PA_EN, 0);
		}
		vTaskDelay(30 / portTICK_RATE_MS);
	}
}
#endif
/*
* Brief:
*	detect the up/dn arrival of elevator 1, and send signal to led task and voice task.
*	waiting until the led task is over
*/
static void task_elevator_1_arrival(void* arg)
{
    uint8_t timecnt_up = 0;
    uint8_t timecnt_dn = 0;
    for(;;) {
    		while(1){
    			if((gpio_get_level(GPIO_DT1_UP) == 0) || (gpio_get_level(GPIO_DT2_UP) == 0) ) {
    				timecnt_up++;
    				if(timecnt_up ==10) break;
    			}
    			else {
    				timecnt_up = 0;
    			} 
    			if((gpio_get_level(GPIO_DT1_DN) ==0 ) || (gpio_get_level(GPIO_DT2_DN) ==0 )){
    				timecnt_dn++;
    				if(timecnt_dn==10) break;
    			}
    			else {
    				timecnt_dn = 0;
    			}
    			vTaskDelay(10 / portTICK_RATE_MS);
    		}
    		if(timecnt_up == 10) {
    			timecnt_up = 0;
    			xSemaphoreGive(xElevator_UpArrival_Semaphore);
    			vTaskDelay(10 / portTICK_RATE_MS);
    			xSemaphoreGive(xVoicePlay_Semaphore);
    			xSemaphoreTake(xLED_Flash_Semaphore, portMAX_DELAY);
    			vTaskDelay(10 / portTICK_RATE_MS);
    		}
    		else {
    			if(timecnt_dn == 10) {
    				timecnt_dn = 0;
    				xSemaphoreGive(xElevator_DnArrival_Semaphore);
    				vTaskDelay(10 / portTICK_RATE_MS);
    				xSemaphoreGive(xVoicePlay_Semaphore);
    				xSemaphoreTake(xLED_Flash_Semaphore, portMAX_DELAY);
    				vTaskDelay(10 / portTICK_RATE_MS);
    			}
    		}

    		vTaskDelay(10 / portTICK_RATE_MS);
    }
}
void led1_flash(void)
{
    uint16_t index, cnt, indey;

    esp_err_t ret;
    spi_transaction_t t;
    uint8_t tmp[96 * 8];
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length= 8 * 96 * 8;                 //Len is in bytes, transaction length is in bits.

    if(xSemaphoreTake(xElevator_UpArrival_Semaphore, 0)) 
    {
	printf("DianTi Up Arrival\n");
	/*
	* TURN OFF ALL LIGHT 
	*/
	for(index = 0; index < 3 * LED_NUM; index++) 
	{
		for(indey = 0; indey < 8; indey++) 
		{
	    		tmp[index*8 + indey] = 0x80;
		}
	}	
		
	t.tx_buffer=&tmp;               //Data
    	ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    	assert(ret==ESP_OK);   
	vTaskDelay(1000 / portTICK_RATE_MS);						

	/*
	* set the lights
	*/
	for(index = 3 * LED_NUM; index > 0; index--) 
	{
	    for(cnt = 0; cnt < 3 * LED_NUM - index + 1; cnt++)
	    {
		    for(indey = 0; indey < 8; indey++) 
		    {
			tmp[cnt * 8 + indey]=0xf0;
		    }
	    }
	    for(; cnt < 3 * LED_NUM; cnt++) 
	    {
		    for(indey = 0; indey < 8; indey++) 
		    {
		    	tmp[cnt * 8 + indey]=0x80;
		    }
	    }

	    t.tx_buffer=&tmp;               //Data
    	    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    	    assert(ret==ESP_OK);   
	    vTaskDelay(50 / portTICK_RATE_MS);						
	}	
	
	xSemaphoreGive(xLED_Flash_Semaphore);		
    }

    if(xSemaphoreTake(xElevator_DnArrival_Semaphore, 0)) 
    {
	printf("DianTi Down Arrival\n");
	/*
	* TURN OFF ALL LIGHT 
	*/
	for(index = 0; index < 3 * LED_NUM; index++) 
	{
		for(indey = 0; indey < 8; indey++) 
		{
	    		tmp[index*8 + indey] = 0x80;
		}
	}	
		
	t.tx_buffer=&tmp;               //Data
    	ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    	assert(ret==ESP_OK);   
	vTaskDelay(1000 / portTICK_RATE_MS);						
	/*
	*	set the light
	*/
	for(index = 0; index < 3 * LED_NUM; index++) 
	{
	    for(cnt = 0; cnt < 3 * LED_NUM - index - 1; cnt++)
	    {
		for(indey = 0; indey < 8; indey++) 
		{
		 	tmp[cnt * 8 + indey]=0x80;
	    	}
	    }
	    for(; cnt < 3 * LED_NUM; cnt++) 
	    {
		for(indey = 0; indey < 8; indey++) 
		{
		 	tmp[cnt * 8 + indey]=0xf0;
	    	}
	    }
	    t.tx_buffer=&tmp;               //Data
    	    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    	    assert(ret==ESP_OK);   
	    vTaskDelay(50 / portTICK_RATE_MS);						
	}	
	/*
	* TURN ON ALL LED LIGHTS 
	*/
	xSemaphoreGive(xLED_Flash_Semaphore);		
    }
}

static void task_led_flash(void* arg)
{
	for(;;)
	{
		led1_flash();
  		vTaskDelay(10 / portTICK_RATE_MS);
	}
}
void app_main()
{
	uint16_t index, cnt;
	uint32_t timecnt;
	gpio_config_t io_conf;
    	esp_err_t ret;
    	uint8_t tmp[96*8];
    	spi_transaction_t t;
    	
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    	io_conf.mode = GPIO_MODE_OUTPUT;
    	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    	io_conf.pull_down_en = 0;
    	io_conf.pull_up_en = 1;
    	gpio_config(&io_conf);

    	io_conf.intr_type = GPIO_PIN_INTR_DISABLE; //GPIO_PIN_INTR_POSEDGE;
    	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    	io_conf.mode = GPIO_MODE_INPUT;
    	io_conf.pull_up_en = 1;
 
     	WRITE_PERI_REG(PIN_CTRL,(CLK_OUT3<<CLK_OUT3_S) | (CLK_OUT2 << CLK_OUT2_S) | (0 << CLK_OUT1_S) );
	PIN_INPUT_DISABLE(PERIPHS_IO_MUX_GPIO0_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    	
    	gpio_config(&io_conf);

    	i2s_setup();
    	spi_bus_config_t buscfg={
        	.miso_io_num=PIN_NUM_MISO,
        	.mosi_io_num=PIN_NUM_MOSI,
        	.sclk_io_num=PIN_NUM_CLK,
        	.quadwp_io_num=-1,
        	.quadhd_io_num=-1,
        	.max_transfer_sz=96 * 8 *16
    	};
    	spi_device_interface_config_t devcfg={
        	.clock_speed_hz=3200000,           //Clock out at 3.2 MHz
        	.mode=0,                                //SPI mode 0
        	.spics_io_num=PIN_NUM_CS,               //CS pin
        	.queue_size=7,                          //We want to be able to queue 7 transactions at a time
    	};
    	//Initialize the SPI bus
    	ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    	ESP_ERROR_CHECK(ret);
    	//Attach the LCD to the SPI bus
    	ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    	//ESP_ERROR_CHECK(ret); 
    	memset(&t, 0, sizeof(t));       //Zero out the transaction
    	t.length= 8 * 96 * 8;                 //Len is in bytes, transaction length is in bits.
    	printf("turn on all the led lights\n");
	for(index = 0; index < 3; index ++ )
	{

		for(cnt = 0; cnt < 3 * LED_NUM *8; cnt++) {
			tmp[cnt] = 0x80;
		}	
		t.tx_buffer=&tmp;               //Data
    		ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    		assert(ret==ESP_OK); 
        	vTaskDelay(1000 / portTICK_RATE_MS);
		for(cnt = 0; cnt < 3 * LED_NUM *8; cnt++) {
			tmp[cnt] = 0xf0;
		}	
		t.tx_buffer=&tmp;               //Data
    		ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    		assert(ret==ESP_OK); 
        	vTaskDelay(1000 / portTICK_RATE_MS);
	}

    	//create Semaphore and Mutex
    	xElevator_UpArrival_Semaphore = xSemaphoreCreateBinary();
    	xElevator_DnArrival_Semaphore = xSemaphoreCreateBinary();
    	xLED_Flash_Semaphore =  xSemaphoreCreateBinary();
	xVoicePlay_Semaphore =  xSemaphoreCreateBinary();	
    	xTaskCreate(task_elevator_1_arrival, "elevator 1 Arrival", 2048, NULL, 10, NULL);
	xTaskCreate(task_led_flash, "led on", 2048, NULL, 11, NULL);
  	xTaskCreate(task_voice_play, "voice play", 2048, NULL, 9, NULL);

    	while(1) 
	{
        	vTaskDelay(10000 / portTICK_RATE_MS);
    	}	
}

