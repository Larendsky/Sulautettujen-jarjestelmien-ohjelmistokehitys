#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Aikamerkkijonon parserointi (HHMMSS)
int time_parse(char *time) {
    if (time == NULL || strlen(time) != 6) {
        return -1;
    }

    int h = (time[0] - '0') * 10 + (time[1] - '0');
    int m = (time[2] - '0') * 10 + (time[3] - '0');
    int s = (time[4] - '0') * 10 + (time[5] - '0');

    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59) {
        return -1;
    }

    // Palautetaan minuutit ja sekunnit yhteensä sekunteina
    return (m * 60) + s;
}

int main(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART initialization failed!\r\n");
        return 0;
    } 

    k_msleep(100);
    printk("Started serial time parser example\n");

    char rc = 0;
    char uart_msg[20];
    int uart_msg_cnt = 0;
    memset(uart_msg, 0, 20);            

    while (true) {
        while (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc != 'X') {
                if (uart_msg_cnt < 19) {
                    uart_msg[uart_msg_cnt] = rc;
                    uart_msg_cnt++;
                }
            } else {
                uart_msg[uart_msg_cnt] = '\0'; // Päätetään merkkijono
                
                int timer_delay = time_parse(uart_msg); 
                
                // Muotoillaan vastaus ja lähetetään takaisin UARTiin
                char response[15];
                snprintf(response, sizeof(response), "%dX", timer_delay);
                
                for(int i = 0; i < strlen(response); i++) {
                    uart_poll_out(uart_dev, response[i]);
                }

                printk("Vastaanotettu: %s, Palautettu: %s\n", uart_msg, response);

                uart_msg_cnt = 0;
                memset(uart_msg, 0, 20);            
            }
        }
        k_msleep(10);
    }
    return 0;
}
