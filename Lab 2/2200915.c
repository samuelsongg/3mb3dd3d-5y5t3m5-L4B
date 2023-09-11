/**
 * Copyright (c) 2023 Fuzzi Pi.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 16
#define UART_RX_PIN 17

const uint BTN_PIN = 15;

int main() {
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_set_pulls(BTN_PIN, true, false);

    char char_data;
    int char_data_int;
    int ascii = 65;

    while (true)
    {
        if(gpio_get(BTN_PIN))
        {
            uart_putc(UART_ID, '1');
        }
        else
        {
            // 65 in ascii decimal represents 'A', 90 represents 'Z'.
            // Increments the ascii decimal so it prints out 'A'-'Z' in a loop.
            uart_putc_raw(UART_ID, (char)ascii);
            ascii++;

            if(ascii > 90)
            {
                ascii = 65;
            }
        }                        

        if(uart_is_readable(UART_ID))
        {
            char_data = uart_getc(UART_ID);
            
            if (char_data == '1')
            {
                char_data = '2';
            }
            else
            {
                // Converts lowercase ascii character to uppercase.
                char_data_int = char_data;
                char_data_int += 32;
                char_data = char_data_int;
            }
            
            printf("%c\n", char_data);
            sleep_ms(1000);
        }
    }

    return 0;
}

/*** end of file ***/