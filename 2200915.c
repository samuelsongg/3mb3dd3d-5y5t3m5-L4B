/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const uint BTN_PIN = 15;
const uint64_t DEBOUNCE_TIME = 200;

volatile int stopwatch_count = 0;
volatile bool stopwatch_started = false;
volatile uint64_t start_time = 0;

struct repeating_timer timer;

// Debouncing algorithm.
// If code runs too fast, debouncing needs to happen.
bool debounce()
{
    uint64_t end_time = to_ms_since_boot(get_absolute_time());
    uint64_t elapsed_time = end_time - start_time;

    if ((elapsed_time) < DEBOUNCE_TIME)
    {
        return true;
    }
    else
    {
        start_time = end_time;
        return false;
    }
}

bool repeating_timer_callback(struct repeating_timer *t)
{
    stopwatch_count++;    
    printf("%ds\n", stopwatch_count);
    return true;
}

// Main stopwatch function.
void gpio_callback(uint gpio, uint32_t events)
{
    // Additional debouncing with stopwatch_started condition.
    // Prevents multiple instances of start/stop from running.
    if (gpio_get(BTN_PIN) && !stopwatch_started)
    {        
        // Returns to main() if debounce is true.
        if (debounce())
        {
            return;
        }

        stopwatch_started = true;
        printf("\nStopwatch has started\n");
        add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer);
    }
    else if (!gpio_get(BTN_PIN) && stopwatch_started)
    {
        if (debounce())
        {
            return;
        }

        stopwatch_started = false;
        printf("\nStopwatch has stopped.\nTotal time elapsed: %d seconds\n", stopwatch_count);
        stopwatch_count = 0;
        cancel_repeating_timer(&timer);
    }

}

int main()
{
    stdio_init_all();
    
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_set_pulls(BTN_PIN, false, true);

    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    while (1);
}

/*** end of file ***/