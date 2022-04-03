/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include <stdint.h>

//#define ORIGINAL_CODE


#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif

const uint DHT_PIN = 15;
const uint MAX_TIMINGS = 85;

#ifdef ORIGINAL_CODE
typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;
void read_from_dht(dht_reading *result);
#else
static volatile bool fire_80us = false;
static uint8_t level_1 = 1;
static uint8_t length_pulses = 40;
static uint8_t data_rxd[40] = {0};
static uint8_t data_global[5] = {0};

int64_t alarm_callback_40us(alarm_id_t id, void *user_data);
int64_t alarm_callback_50us(alarm_id_t id, void *user_data);
int64_t alarm_callback_160us(alarm_id_t id, void *user_data);
static void convert_data_temp_hum(void);
static void newlogic_read_dht(void);
#endif

#ifdef ORIGINAL_CODE 
void read_from_dht(dht_reading *result) {
    int data[5] = {0, 0, 0, 0, 0};
    uint last = 1;
    uint j = 0;

    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(20);
    gpio_set_dir(DHT_PIN, GPIO_IN);

#ifdef LED_PIN
    gpio_put(LED_PIN, 1);
#endif
    for (uint i = 0; i < MAX_TIMINGS; i++)
    {
        uint count = 0;
        while (gpio_get(DHT_PIN) == last)
        {
            count++;
            sleep_us(1);
            if (count == 255) 
                break;
        }
        last = gpio_get(DHT_PIN);
        if (count == 255) 
            break;
        if ((i >= 4) && (i % 2 == 0)) 
        {
            data[j / 8] <<= 1;
            if (count > 16)
                data[j / 8] |= 1;
            j++;
        }
    }
#ifdef LED_PIN
    gpio_put(LED_PIN, 0);
#endif
    // Decoding of data but here we want clear data
    #if 1 
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) 
    {
        result->humidity = (float) ((data[0] << 8) + data[1]) / 10;
        if (result->humidity > 100) {
            result->humidity = data[0];
        }
        result->temp_celsius = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (result->temp_celsius > 125) {
            result->temp_celsius = data[2];
        }
        if (data[2] & 0x80) {
            result->temp_celsius = -result->temp_celsius;
        }
    }
    else {
        printf("\n data[0]=%d data[1]=%d data[2]=%d data[3]=%d data[4]=%d ", 
                data[0], data[1],data[2],data[3],data[4]);
    }
    #endif
}
#else

static void convert_data_temp_hum(void)
{
    uint8_t i,j=0;
    uint8_t shiftc = 8;
    uint8_t temp = 0;
    // actual logic
    for(i=0;i<length_pulses;i++)
    {
        temp=0;
        // modify the data & store it in temp buffer
        if(data_rxd[i] > 24)
        {
            temp |= level_1;
            temp <<= shiftc--;
            data_global[j/8] |= temp;
        }
        else
        {
            shiftc--;         
        }
        j++;
        if((j%8) == 0)
            shiftc = 8;     
    }
}

int64_t alarm_callback_50us(alarm_id_t id, void *user_data)
{
    uint8_t loop=0;
    uint8_t i=0;
    while(gpio_get(DHT_PIN) == false)
    { 
        busy_wait_us(1);
    }
    
    // introduce for loop to get 84 pulse counts
    for(loop=0;loop<length_pulses;loop++)
    {
        i=0;
        // checking needed or not
        sleep_us(1);
        while(gpio_get(DHT_PIN) == true)
        {
            i++;
            sleep_us(1);
            if(i == 255)
            {
                break;
            }
            data_rxd[loop] = i;
        }
        while(gpio_get(DHT_PIN) == false)
        {
            sleep_us(1);
        }
    }

    return 0;
}

int64_t alarm_callback_160us(alarm_id_t id, void *user_data)
{
    uint i=0;
    while(gpio_get(DHT_PIN) == true)
    {
        i++;
        busy_wait_us(1);
        if(i == 255)
        {
            break;
        }
    }
    
    add_alarm_in_us(50, alarm_callback_50us, NULL, false);
    fire_80us = true;    
    return 0;
}

int64_t alarm_callback_40us(alarm_id_t id, void *user_data) 
{
    return 0;
}

static void newlogic_read_dht(void)
{
    #ifdef LED_PIN
        gpio_put(LED_PIN, 1);
    #endif     
    // check by default what state of GPIO of PICO
    if(gpio_is_pulled_down(DHT_PIN))
    {
        // by default its pull-down confirmed here
        gpio_set_pulls(DHT_PIN,true,false);
    }
    if(gpio_is_pulled_up(DHT_PIN))
    {
        gpio_set_dir(DHT_PIN, GPIO_OUT);
        gpio_put(DHT_PIN, 0);
        sleep_ms(18);
        gpio_put(DHT_PIN, 1);
        gpio_set_dir(DHT_PIN, GPIO_IN);
        // by default its pull-down confirmed here
        gpio_set_pulls(DHT_PIN,true,false);
   
        // Call alarm_callback in 40 seconds
        add_alarm_in_us(40, alarm_callback_40us, NULL, false);
        add_alarm_in_us(80, alarm_callback_160us, NULL, false);
        sleep_ms(1);
        while(!fire_80us);
        fire_80us = false;
    #ifdef LED_PIN
        gpio_put(LED_PIN, 0);
    #endif
    }
}
#endif

int main() {
    stdio_init_all();  
#ifdef LED_PIN
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif
    while (1) 
    {   
#ifdef ORIGINAL_CODE         
        dht_reading reading = {0.0, 0.0};
        read_from_dht(&reading);
#else
        gpio_init(DHT_PIN);
        newlogic_read_dht();
#endif
        convert_data_temp_hum();
        if ((data_global[4] == ((data_global[0] + data_global[1] + data_global[2] + data_global[3]) & 0xFF)))
        {
            float hum = (float)data_global[0] + (float)(data_global[1]/10.0);
            float temp123 = (float)data_global[2] + (float)(data_global[3]/10.0);
            printf(" \n h= %f & t= %f \n ", hum, temp123);            
        }        
        for(uint8_t loop=0; loop<5; loop++)
        {
            data_global[loop] = 0;
        }
         for(uint8_t loop=0;loop<length_pulses;loop++)
            data_rxd[loop] = 0; 

        sleep_ms(2000);
    }
}