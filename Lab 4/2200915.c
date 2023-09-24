/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

struct repeating_timer timer;

bool repeating_timer_callback(struct repeating_timer *t)
{   
    // Get current time in ms.
    uint64_t current_time = to_ms_since_boot(get_absolute_time());
    // Read ADC value.
    uint64_t result = adc_read();
    printf("Time in ms: %llu -> ADC Value: %d\n", current_time, result);
    return true;   
}

int main()
{
    stdio_init_all();
    adc_init();

    // Set GP26 as ADC.
    adc_gpio_init(26);
    // Set ADC channel 0.
    adc_select_input(0);

    // Set GP2 as PWM.
    gpio_set_function(2, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(2);
    
    // Set clock divisor to 100, reduce main clock to 1.25Mhz from 125Mhz.
    pwm_set_clkdiv(slice_num, 100);
    // Set period to 62500 cycles.
    // 1,250,000 (1.25Mhz) / 62500 = 20hz frequency.
    pwm_set_wrap(slice_num, 62500);
    // Set duty cycle to 50%.
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 62500 / 2);
    pwm_set_enabled(slice_num, true);
    
    // Set timer to sample PWM every 25ms.
    add_repeating_timer_ms(25, repeating_timer_callback, NULL, &timer);

    while (1);
}

/*** end of file ***/