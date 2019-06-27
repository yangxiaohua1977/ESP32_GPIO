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

/**
 * Brief:
 * Detect the evevator up/dn arrival, then, turn on and flash the LED lights, play the voice
 *
 */

#define LED_NUM	    32 

#define ZERO_CODE_H   8
#define ZERO_CODE_L   46

#define ONE_CODE_H    25
#define ONE_CODE_L    27

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


extern void voice_play(void);
extern void i2s_setup(void);

SemaphoreHandle_t xElevator1_UpArrival_Semaphore = NULL;
SemaphoreHandle_t xElevator1_DnArrival_Semaphore = NULL;
SemaphoreHandle_t xElevator2_UpArrival_Semaphore = NULL;
SemaphoreHandle_t xElevator2_DnArrival_Semaphore = NULL;
SemaphoreHandle_t xLED1_Flash_Semaphore = NULL;
SemaphoreHandle_t xLED2_Flash_Semaphore = NULL;
SemaphoreHandle_t xVoicePlay_Semaphore = NULL;
SemaphoreHandle_t xVoicePlay_Mutex = NULL;

	
/*
* Brief:
* 	when semaphore is activity, Play the voice via I2S
*/
static void task_voice_play(void* arg)
{
	for(;;)
	{
			if(xSemaphoreTake(xVoicePlay_Semaphore, 0)) 
			{
				gpio_set_level(GPIO_PA_EN, 1);
				voice_play();
				gpio_set_level(GPIO_PA_EN, 0);
				xSemaphoreGive(xVoicePlay_Mutex);
			}
			vTaskDelay(30 / portTICK_RATE_MS);
	}
}
/*
* Brief:
*	detect the up/dn arrival of elevator 1, and send signal to led task and voice task.
*	waiting until the led task is over
*/
static void task_elevator_1_arrival(void* arg)
{
    uint8_t timecnt_up = 0;
    uint8_t timecnt_up2 = 0;
    uint8_t timecnt_dn = 0;
    uint8_t timecnt_dn2 = 0;
    for(;;) {
    		while(1){
    			if(gpio_get_level(GPIO_DT1_UP) == 0) {
    				vTaskDelay(10 / portTICK_RATE_MS);
    				timecnt_up++;
    				if(timecnt_up ==10) break;
    			}
    			else {
    				timecnt_up = 0;
    			} 
    			if(gpio_get_level(GPIO_DT1_DN) ==0 ) {
    				vTaskDelay(10 / portTICK_RATE_MS);
    				timecnt_dn++;
    				if(timecnt_dn==10) break;
    			}
    			else {
    				timecnt_dn = 0;
    			}

    			if(gpio_get_level(GPIO_DT2_UP) == 0) {
    				vTaskDelay(10 / portTICK_RATE_MS);
    				timecnt_up2++;
    				if(timecnt_up2 ==10) break;
    			}
    			else {
    				timecnt_up2 = 0;
    			} 
    			if(gpio_get_level(GPIO_DT2_DN) == 0) {
    				vTaskDelay(10 / portTICK_RATE_MS);
    				timecnt_dn2++;
    				if(timecnt_dn2==10) break;
    			}
    			else {
    				timecnt_dn2 = 0;
    			}
    		}
    		if(timecnt_up == 10) {
    			timecnt_up = 0;
    			xSemaphoreGive(xElevator1_UpArrival_Semaphore);
			xSemaphoreTake(xVoicePlay_Mutex, portMAX_DELAY);
    			xSemaphoreGive(xVoicePlay_Semaphore);
			//xSemaphoreGive(xVoicePlay_Mutex);
    			vTaskDelay(100 / portTICK_RATE_MS);
    			xSemaphoreTake(xLED1_Flash_Semaphore, portMAX_DELAY);
    			vTaskDelay(100 / portTICK_RATE_MS);
    		}
    		else {
    			if(timecnt_dn == 10) {
    				timecnt_dn = 0;
    				xSemaphoreGive(xElevator1_DnArrival_Semaphore);
				xSemaphoreTake(xVoicePlay_Mutex, portMAX_DELAY);
    				xSemaphoreGive(xVoicePlay_Semaphore);
				//xSemaphoreGive(xVoicePlay_Mutex);
    				vTaskDelay(100 / portTICK_RATE_MS);
    				xSemaphoreTake(xLED1_Flash_Semaphore, portMAX_DELAY);
    				vTaskDelay(100 / portTICK_RATE_MS);
    			}
			else
			{

    				if(timecnt_up2 == 10) {
    					timecnt_up2 = 0;
    					xSemaphoreGive(xElevator2_UpArrival_Semaphore);
					xSemaphoreTake(xVoicePlay_Mutex, portMAX_DELAY);
    					xSemaphoreGive(xVoicePlay_Semaphore);
					//xSemaphoreGive(xVoicePlay_Mutex);
    					vTaskDelay(100 / portTICK_RATE_MS);
    					xSemaphoreTake(xLED2_Flash_Semaphore, portMAX_DELAY);
    					vTaskDelay(100 / portTICK_RATE_MS);
    				}
    				else {
    					if(timecnt_dn2 == 10) {
    						timecnt_dn2 = 0;
    						xSemaphoreGive(xElevator2_DnArrival_Semaphore);
						xSemaphoreTake(xVoicePlay_Mutex, portMAX_DELAY);
    						xSemaphoreGive(xVoicePlay_Semaphore);
						//xSemaphoreGive(xVoicePlay_Mutex);
    						vTaskDelay(100 / portTICK_RATE_MS);
    						xSemaphoreTake(xLED2_Flash_Semaphore, portMAX_DELAY);
    						vTaskDelay(100 / portTICK_RATE_MS);
    					}	
    				}
			}
    		}

    		vTaskDelay(50 / portTICK_RATE_MS);
    }
}
/*
* Brief:
* 	detect the up/dn arrival of elevator 2, and send signal to led task and voice task 
* 	wating until led task is over
*/
static void task_elevator_2_arrival(void* arg)
{
    uint8_t timecnt_up = 0;
    uint8_t timecnt_dn = 0;
    for(;;) {
    		while(1){
    			if(gpio_get_level(GPIO_DT2_UP) == 0) {
    				vTaskDelay(10 / portTICK_RATE_MS);
    				timecnt_up++;
    				if(timecnt_up ==10) break;
    			}
    			else {
    				timecnt_up = 0;
    			} 
    			if(gpio_get_level(GPIO_DT2_DN) == 0) {
    				vTaskDelay(10 / portTICK_RATE_MS);
    				timecnt_dn++;
    				if(timecnt_dn==10) break;
    			}
    			else {
    				timecnt_dn = 0;
    			}
    		}
    		if(timecnt_up == 10) {
    			timecnt_up = 0;
    			xSemaphoreGive(xElevator2_UpArrival_Semaphore);
			xSemaphoreTake(xVoicePlay_Mutex, portMAX_DELAY);
    			xSemaphoreGive(xVoicePlay_Semaphore);
			//xSemaphoreGive(xVoicePlay_Mutex);
    			vTaskDelay(100 / portTICK_RATE_MS);
    			xSemaphoreTake(xLED2_Flash_Semaphore, portMAX_DELAY);
    			vTaskDelay(100 / portTICK_RATE_MS);
    		}
    		else {
    			if(timecnt_dn == 10) {
    				timecnt_dn = 0;
    				xSemaphoreGive(xElevator2_DnArrival_Semaphore);
				xSemaphoreTake(xVoicePlay_Mutex, portMAX_DELAY);
    				xSemaphoreGive(xVoicePlay_Semaphore);
				//xSemaphoreGive(xVoicePlay_Mutex);
    				vTaskDelay(100 / portTICK_RATE_MS);
    				xSemaphoreTake(xLED2_Flash_Semaphore, portMAX_DELAY);
    				vTaskDelay(100 / portTICK_RATE_MS);
    			}
    		}
    		vTaskDelay(150 / portTICK_RATE_MS);
    }
}
/*
* Brief:
* 	led task for elevator 1
* 	if the semaphore is activity, the led turned off firstly, then it will be turned on in turn.
* 	all the lights will be turn on at last 
*/
static void task_led1_flash(void* arg)
{
    uint8_t index, indey, datmp, bitcnt;
    uint32_t timecnt;
    for(;;) {
    		/*
    		* elevator up
    		*/
	if(xSemaphoreTake(xElevator1_UpArrival_Semaphore, 0)) 
	{
		printf("DianTi Up Arrival\n");
		/*
		* TURN OFF ALL LIGHT 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(index==(LED_NUM-1))
				{
					datmp = 0x00;
				}
				else { 
					datmp = 0xFF;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
		}						
		/*
		* set the lights
		*/
		for(index =LED_NUM; index > 0; index--)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(indey < 3 * (index-1)) 
				{
					datmp = 0x00;
				}
				else { 
					datmp = 0xFF;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt = 0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
			vTaskDelay(150 / portTICK_RATE_MS);						
		}
		/*
		* TURN ON ALL LIGHT 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				datmp = 0xff;
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}		
		}
		xSemaphoreGive(xLED1_Flash_Semaphore);		
  	}
	/*
	* elevator down
	*/
	if(xSemaphoreTake(xElevator1_DnArrival_Semaphore, 0)) 
	{
		printf("DianTi Down Arrival\n");
		/*
		* TURN OFF ALL LIGHT 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(index==(LED_NUM-1)) 
				{
					datmp = 0x00;
				}
				else {
					datmp = 0xff;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
		}							
		/*
		*	set the light
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(indey < 3 * (index+1)) 
				{
					datmp = 0xFF;
				}
				else {
					datmp = 0x00;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
			vTaskDelay(150 / portTICK_RATE_MS);						
		}
		/*
		* TURN ON ALL LED LIGHTS 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				datmp = 0xff;
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_UP, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_UP, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
		}
		xSemaphoreGive(xLED1_Flash_Semaphore);		
  	}
  	vTaskDelay(200 / portTICK_RATE_MS);
    }
}
/*
* Brief:
* 	led task for elevator 2
*/
static void task_led2_flash(void* arg)
{
    uint8_t index, indey, datmp, bitcnt;
    uint32_t timecnt;
    for(;;) {
    		/*
    		* elevator up
    		*/
	if(xSemaphoreTake(xElevator2_UpArrival_Semaphore, 0)) 
	{
		printf("DianTi Up Arrival\n");
		/*
		* TURN OFF ALL LIGHT 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(index==(LED_NUM-1))
				{
					datmp = 0x00;
				}
				else { 
					datmp = 0xFF;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
		}						
		/*
		* set the lights
		*/
		for(index =LED_NUM; index > 0; index--)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(indey < 3 * (index-1)) 
				{
					datmp = 0x00;
				}
				else { 
					datmp = 0xFF;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt = 0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
			vTaskDelay(150 / portTICK_RATE_MS);						
		}
		/*
		* TURN ON ALL LIGHT 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				datmp = 0xff;
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}		
		}
		xSemaphoreGive(xLED2_Flash_Semaphore);		
  	}
	/*
	* elevator down
	*/
	if(xSemaphoreTake(xElevator2_DnArrival_Semaphore, 0)) 
	{
		printf("DianTi Down Arrival\n");
		/*
		* TURN OFF ALL LIGHT 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(index==(LED_NUM-1)) 
				{
					datmp = 0x00;
				}
				else {
					datmp = 0xff;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
		}							
		/*
		*	set the light
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				if(indey < 3 * (index+1)) 
				{
					datmp = 0xFF;
				}
				else {
					datmp = 0x00;
				}
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
			vTaskDelay(150 / portTICK_RATE_MS);						
		}
		/*
		* TURN ON ALL LED LIGHTS 
		*/
		for(index =0; index < LED_NUM; index++)
		{
			for(indey = 0; indey < 3 * LED_NUM; indey++) 
			{
				datmp = 0xff;
				for(bitcnt = 0; bitcnt <8; bitcnt++) 
				{
					if((datmp << bitcnt) & 0x80) 
					{
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
						{
							;
						}
					}
					else {
						gpio_set_level(GPIO_LED_DN, 0);
						for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
						{
							;
						}
						gpio_set_level(GPIO_LED_DN, 1);
						for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
						{
							;
						}										
					}
				}
			}
		}
		xSemaphoreGive(xLED2_Flash_Semaphore);		
  	}
  	vTaskDelay(100 / portTICK_RATE_MS);
    }
}
void app_main()
{
	uint8_t index, indey, datmp, bitcnt;
	uint32_t timecnt;
	gpio_config_t io_conf;
	//disable interrupt
    	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    	//set as output mode
    	io_conf.mode = GPIO_MODE_OUTPUT;
    	//bit mask of the pins that you want to set,e.g.GPIO18/19
    	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
    	io_conf.pull_down_en = 0;
    	//disable pull-up mode
    	io_conf.pull_up_en = 1;
    	//configure GPIO with the given settings
    	gpio_config(&io_conf);

    	//interrupt of rising edge
    	io_conf.intr_type = GPIO_PIN_INTR_DISABLE; //GPIO_PIN_INTR_POSEDGE;
    	//bit mask of the pins, use GPIO4/5 here
    	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    	//set as input mode    
    	io_conf.mode = GPIO_MODE_INPUT;
    	//enable pull-up mode
    	io_conf.pull_up_en = 1;
 
     	WRITE_PERI_REG(PIN_CTRL,(CLK_OUT3<<CLK_OUT3_S) | (CLK_OUT2 << CLK_OUT2_S) | (0 << CLK_OUT1_S) );
	PIN_INPUT_DISABLE(PERIPHS_IO_MUX_GPIO0_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    	
    	gpio_config(&io_conf);

    	i2s_setup();
    	printf("turn on all the led lights\n");
	for(index =0; index < LED_NUM; index++)
	{
		for(indey = 0; indey < 3 * LED_NUM; indey++) 
		{
			datmp = 0xFF;
			for(bitcnt = 0; bitcnt <8; bitcnt++) 
			{
				if((datmp << bitcnt) & 0x80) 
				{
					gpio_set_level(GPIO_LED_UP, 0);
					gpio_set_level(GPIO_LED_DN, 0);
					for(timecnt =0; timecnt < ONE_CODE_H; timecnt++)
					{
						;
					}
					gpio_set_level(GPIO_LED_UP, 1);
					gpio_set_level(GPIO_LED_DN, 1);
					for(timecnt =0; timecnt < ONE_CODE_L; timecnt++)
					{
						;
					}
				}
				else {
					gpio_set_level(GPIO_LED_UP, 0);
					gpio_set_level(GPIO_LED_DN, 0);
					for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++)
					{
						;
					}
					gpio_set_level(GPIO_LED_UP, 1);
					gpio_set_level(GPIO_LED_DN, 1);
					for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++)
					{
						;
					}										
				}
			}
		}
	}

    	//create Semaphore and Mutex
    	xElevator1_UpArrival_Semaphore = xSemaphoreCreateBinary();
    	xElevator1_DnArrival_Semaphore = xSemaphoreCreateBinary();
    	xLED1_Flash_Semaphore =  xSemaphoreCreateBinary();
    	xElevator2_UpArrival_Semaphore = xSemaphoreCreateBinary();
    	xElevator2_DnArrival_Semaphore = xSemaphoreCreateBinary();
    	xLED2_Flash_Semaphore =  xSemaphoreCreateBinary();
	xVoicePlay_Semaphore =  xSemaphoreCreateBinary();	
	xVoicePlay_Mutex = xSemaphoreCreateMutex();	
    	//start gpio task
    	xTaskCreate(task_elevator_1_arrival, "elevator 1 Arrival", 2048, NULL, 11, NULL);
    	//xTaskCreate(task_elevator_2_arrival, "elevator 2 Arrival", 2048, NULL, 10, NULL);
	xTaskCreate(task_led1_flash, "led on", 2048, NULL, 9, NULL);
	xTaskCreate(task_led2_flash, "led on", 2048, NULL, 8, NULL);
  	xTaskCreate(task_voice_play, "voice play", 2048, NULL, 12, NULL);

    	while(1) 
	{
        	vTaskDelay(1000 / portTICK_RATE_MS);
    	}	
}

