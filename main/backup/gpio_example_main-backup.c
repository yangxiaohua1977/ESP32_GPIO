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

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define LED_NUM	    3 

#define ZERO_CODE_H   8
#define ZERO_CODE_L   46

#define ONE_CODE_H    25
#define ONE_CODE_L    27

#define GPIO_UP_P    32
#define GPIO_UP_N    33

#define GPIO_DN_P    14
#define GPIO_DN_N    12

#define GPIO_LED_UP    22
#define GPIO_LED_DN    21


#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_LED_UP) | (1ULL<<GPIO_LED_DN))

#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_UP_P) | (1ULL<<GPIO_UP_N)|(1ULL<<GPIO_DN_P) | (1ULL<<GPIO_DN_N))
#define ESP_INTR_FLAG_DEFAULT 0

SemaphoreHandle_t xElevator_UpArrival_Semaphore = NULL;
SemaphoreHandle_t xElevator_DnArrival_Semaphore = NULL;
SemaphoreHandle_t xLED_Flash_Semaphore = NULL;

/*
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
*/

static void task_elevator_up_arrival(void* arg)
{
    for(;;) {
    		if(gpio_get_level(GPIO_UP_P) ^ gpio_get_level(GPIO_UP_N)) {
    			xSemaphoreGive(xElevator_UpArrival_Semaphore);
    			vTaskDelay(1500 / portTICK_RATE_MS);
    			xSemaphoreTake(xLED_Flash_Semaphore, portMAX_DELAY);
    			vTaskDelay(5000 / portTICK_RATE_MS);
    		}
    }
}
static void task_elevator_dn_arrival(void* arg)
{
    for(;;) {
    		if((gpio_get_level(GPIO_DN_P) ^ gpio_get_level(GPIO_DN_N)) == 0) {
    			xSemaphoreGive(xElevator_DnArrival_Semaphore);
    			vTaskDelay(1500 / portTICK_RATE_MS);
    			xSemaphoreTake(xLED_Flash_Semaphore, portMAX_DELAY);
    			vTaskDelay(5000 / portTICK_RATE_MS);
    		}
    }
}
static void task_led_flash(void* arg)
{
    uint8_t index, indey, datmp, bitcnt;
    uint32_t timecnt;
    for(;;) {
    			/*
    			* elevator up
    			*/
					if(xSemaphoreTake(xElevator_UpArrival_Semaphore, 0)) {
						/*
						*	set the light
						*/
						for(index =0; index < LED_NUM; index++){
							for(indey = 0; indey < 3 * LED_NUM; indey++) {
								if(indey >= 3 * LED_NUM-3-index*3) {
									datmp = 0x7f;
								}
								else{
									datmp = 0;
								}
								for(bitcnt = 0; bitcnt <8; bitcnt++) {
									if((datmp << bitcnt) & 0x80) {
										gpio_set_level(GPIO_LED_UP, 1);
										for(timecnt =0; timecnt < ONE_CODE_H; timecnt++){
											;
										}
										gpio_set_level(GPIO_LED_UP, 0);
										for(timecnt =0; timecnt < ONE_CODE_L; timecnt++){
											;
										}
									}
									else {
										gpio_set_level(GPIO_LED_UP, 1);
										for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++){
											;
										}
										gpio_set_level(GPIO_LED_UP, 0);
										for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++){
											;
										}										
									}
								}
								vTaskDelay(1000 / portTICK_RATE_MS);
							}
							vTaskDelay(1000 / portTICK_RATE_MS);
						}	
						xSemaphoreGive(xLED_Flash_Semaphore);									
					}
					/*
					* elevator down
					*/
					if(xSemaphoreTake(xElevator_DnArrival_Semaphore, 0)) {
						/*
						*	set the light
						*/
						uint8_t cnt; 
						for(cnt =0; cnt <10; cnt++) {
						for(index =0; index < LED_NUM; index++){
							for(indey = 0; indey < 3 * LED_NUM; indey++) {
								if(indey < 3 * index*+3) {
									datmp = 0x0;
								}
								else{
									datmp = 0x7f;
								}
								for(bitcnt = 0; bitcnt <8; bitcnt++) {
									if((datmp << bitcnt) & 0x80) {
										gpio_set_level(GPIO_LED_UP, 0);
										for(timecnt =0; timecnt < ONE_CODE_H; timecnt++){
											;
										}
										gpio_set_level(GPIO_LED_UP, 1);
										for(timecnt =0; timecnt < ONE_CODE_L; timecnt++){
											;
										}
									}
									else {
										gpio_set_level(GPIO_LED_UP, 0);
										for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++){
											;
										}
										gpio_set_level(GPIO_LED_UP, 1);
										for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++){
											;
										}										
									}
								}
								//vTaskDelay(10 / portTICK_RATE_MS);
							}
							vTaskDelay(1000 / portTICK_RATE_MS);						
						}
						/*
						* Clear 
						*/
						for(index =0; index < LED_NUM; index++){
							for(indey = 0; indey < 3 * LED_NUM; indey++) {
								if(indey < 3 * index*+3) {
									datmp = 0xff;
								}
								else{
									datmp = 0xff;
								}
								for(bitcnt = 0; bitcnt <8; bitcnt++) {
									if((datmp << bitcnt) & 0x80) {
										gpio_set_level(GPIO_LED_UP, 1);
										for(timecnt =0; timecnt < ONE_CODE_H; timecnt++){
											;
										}
										gpio_set_level(GPIO_LED_UP, 0);
										for(timecnt =0; timecnt < ONE_CODE_L; timecnt++){
											;
										}
									}
									else {
										gpio_set_level(GPIO_LED_UP, 1);
										for(timecnt =0; timecnt < ZERO_CODE_H; timecnt++){
											;
										}
										gpio_set_level(GPIO_LED_UP, 0);
										for(timecnt =0; timecnt < ZERO_CODE_L; timecnt++){
											;
										}										
									}
								}
								//vTaskDelay(10 / portTICK_RATE_MS);
							}
							vTaskDelay(1000 / portTICK_RATE_MS);						
						}
						
						vTaskDelay(1000 / portTICK_RATE_MS);	
					}
					xSemaphoreGive(xLED_Flash_Semaphore);		
    		}
    }
}

void app_main()
{
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
    io_conf.pull_up_en = 0;
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
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    //gpio_set_intr_type(GPIO_UP_P, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    xElevator_UpArrival_Semaphore = xSemaphoreCreateBinary();
    xElevator_DnArrival_Semaphore = xSemaphoreCreateBinary();
    xLED_Flash_Semaphore =  xSemaphoreCreateBinary();

    //start gpio task
    xTaskCreate(task_elevator_up_arrival, "elevator Up Arrival", 2048, NULL, 10, NULL);
    xTaskCreate(task_elevator_dn_arrival, "elevator Dn Arrival", 2048, NULL, 10, NULL);
 		xTaskCreate(task_led_flash, "led on", 2048, NULL, 11, NULL);
    //install gpio isr service
    //gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    //gpio_isr_handler_add(GPIO_UP_P, gpio_isr_handler, (void*) GPIO_UP_P);
    //hook isr handler for specific gpio pin
    //gpio_isr_handler_add(GPIO_DN_P, gpio_isr_handler, (void*) GPIO_DN_P);

    //remove isr handler for gpio number.
    //gpio_isr_handler_remove(GPIO_UP_P);
    //hook isr handler for specific gpio pin again
    //gpio_isr_handler_add(GPIO_UP_P, gpio_isr_handler, (void*) GPIO_UP_P);

    //int cnt = 0;
    while(1) {
       // printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_RATE_MS);
        //gpio_set_level(GPIO_LED_UP, cnt % 2);
        //gpio_set_level(GPIO_LED_DN, cnt % 2);
    }
}

