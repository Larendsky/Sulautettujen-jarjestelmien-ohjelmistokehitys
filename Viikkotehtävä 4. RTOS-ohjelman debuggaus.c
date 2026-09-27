//tavoittelen 3p: ajoitukset liikennevaloihin, Lisää ajoitustietoja ja Debugin asetus päälle / pois
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/timing/timing.h>

// Laitteisto (Napit ja Ledit)
#define BUTTON_0 DT_ALIAS(sw0)

// Dynaaminen debug-lippu (Debugin asetus päälle/pois)
bool debug_enabled = true; 

static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec yellow = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// UART
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// RTOS Työkalut
K_FIFO_DEFINE(dispatcher_fifo);
struct data_t {
    void *fifo_reserved;
    char msg[20];
};

// Valofifot ja synkronointi
K_SEM_DEFINE(release_sem, 0, 1);
K_FIFO_DEFINE(red_fifo);
K_FIFO_DEFINE(yellow_fifo);
K_FIFO_DEFINE(green_fifo);

struct color_data_t {
    void *fifo_reserved;
    int time_ms;
};

volatile bool is_paused = false;

// PAUSE
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    is_paused = !is_paused;
    printk("Pause painettu\n");
}

// Alustukset
int init_hardware(void) {
    if (!device_is_ready(uart_dev)) return -1;
    
    gpio_pin_configure_dt(&button_0, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_data);

    gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&red, 0);
    gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&green, 0);
    gpio_pin_configure_dt(&yellow, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&yellow, 0);

    return 0;
}

int main(void) {
    init_hardware();
    timing_init();
    timing_start();
    printk("Odotetaan komentoja (esim. R,1000 tai D debugille)...\n");
    return 0;
}

// TASKIT

// UART
static void uart_task(void *a, void *b, void *c) {
    char rc = 0;
    char uart_msg[20];
    memset(uart_msg, 0, 20);
    int uart_msg_cnt = 0;

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc != '\r' && rc != '\n') {
                if (uart_msg_cnt < 19) {
                    uart_msg[uart_msg_cnt] = rc;
                    uart_msg_cnt++;
                }
            } 
            else if (uart_msg_cnt > 0) {
                // tarkistetaan onko kyseessä debug-komento 'D' tai 'd'
                if (uart_msg[0] == 'D' || uart_msg[0] == 'd') {
                    debug_enabled = !debug_enabled;
                    if (debug_enabled) {
                        printk(">>> Debug-tulosteet: PALLÄ <<<\n");
                    } else {
                        printk(">>> Debug-tulosteet: POIS <<<\n");
                    }
                } else {
                    if (debug_enabled) {
                        printk("Vastaanotettu sana: %s\n", uart_msg);
                    }
                    
                    struct data_t *buf = k_malloc(sizeof(struct data_t));
                    if (buf != NULL) {
                        strcpy(buf->msg, uart_msg);
                        k_fifo_put(&dispatcher_fifo, buf);
                    }
                }

                uart_msg_cnt = 0;
                memset(uart_msg, 0, 20);
            }
        }
        k_msleep(10);
    }
}

// Dispatcher eli työnjohtaja (Lisää ajoitustietoja)
static void dispatcher_task(void *a, void *b, void *c) {
    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        
        // Mitataan Dispatcherin käsittelyaika
        timing_t disp_start = timing_counter_get();

        char sequence[20];
        strcpy(sequence, rec_item->msg);
        k_free(rec_item);

        char color = sequence[0];
        int time = atoi(sequence + 2);

        if (debug_enabled) {
            printk("Purettu komento: Valo %c, Aika %d ms\n", color, time);
        }

        while (is_paused) {
            k_msleep(100); 
        }

        struct color_data_t *c_data = k_malloc(sizeof(struct color_data_t));
        if (c_data != NULL) {
            c_data->time_ms = time;
            
            if (color == 'R' || color == 'r') {
                k_fifo_put(&red_fifo, c_data);
            } else if (color == 'Y' || color == 'y') {
                k_fifo_put(&yellow_fifo, c_data);
            } else if (color == 'G' || color == 'g') {
                k_fifo_put(&green_fifo, c_data);
            } else {
                k_free(c_data);
                continue;
            }
            
            timing_t disp_end = timing_counter_get();
            uint64_t disp_ns = timing_cycles_to_ns(timing_cycles_get(&disp_start, &disp_end));
            
            if (debug_enabled) {
                printk("Dispatcher käsittelyaika: %llu us\n", (unsigned long long)(disp_ns / 1000));
            }

            k_sem_take(&release_sem, K_FOREVER);
        }
    }
}

// Valotaskit mittauksilla
void red_task(void *a, void *b, void *c) {
    while (true) {
        struct color_data_t *data = k_fifo_get(&red_fifo, K_FOREVER);
        int sleep_time = data->time_ms;
        k_free(data);
        
        timing_t start = timing_counter_get();

        gpio_pin_set_dt(&red, 1);
        
        if (debug_enabled) {
            printk("Red led on\n");
        }

        k_msleep(sleep_time);
        
        gpio_pin_set_dt(&red, 0);   

        if (debug_enabled) {
            printk("Red led off\n");
        }

        timing_t end = timing_counter_get();
        uint64_t ns = timing_cycles_to_ns(timing_cycles_get(&start, &end));
        
        printk("Red task kesto: %llu us\n", (unsigned long long)(ns / 1000));
        
        k_sem_give(&release_sem);
    }
}

void yellow_task(void *a, void *b, void *c) {
    while (true) {
        struct color_data_t *data = k_fifo_get(&yellow_fifo, K_FOREVER);
        int sleep_time = data->time_ms;
        k_free(data); 
        
        timing_t start = timing_counter_get();

        gpio_pin_set_dt(&yellow, 1);
        
        if (debug_enabled) {
            printk("Yellow led on\n");
        }

        k_msleep(sleep_time);
        
        gpio_pin_set_dt(&yellow, 0);   

        if (debug_enabled) {
            printk("Yellow led off\n");
        }

        timing_t end = timing_counter_get();
        uint64_t ns = timing_cycles_to_ns(timing_cycles_get(&start, &end));
        
        printk("Yellow task kesto: %llu us\n", (unsigned long long)(ns / 1000));
        
        k_sem_give(&release_sem); 
    }
}

void green_task(void *a, void *b, void *c) {
    while (true) {
        struct color_data_t *data = k_fifo_get(&green_fifo, K_FOREVER);
        int sleep_time = data->time_ms;
        k_free(data); 
        
        timing_t start = timing_counter_get();

        gpio_pin_set_dt(&green, 1);
        
        if (debug_enabled) {
            printk("Green led on\n");
        }

        k_msleep(sleep_time);
        
        gpio_pin_set_dt(&green, 0);   

        if (debug_enabled) {
            printk("Green led off\n");
        }

        timing_t end = timing_counter_get();
        uint64_t ns = timing_cycles_to_ns(timing_cycles_get(&start, &end));
        
        printk("Green task kesto: %llu us\n", (unsigned long long)(ns / 1000));
        
        k_sem_give(&release_sem); 
    }
}

// Käynnistykset
#define STACKSIZE 1024
#define PRIORITY 5
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dis_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(red_thread, STACKSIZE, red_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_task, NULL, NULL, NULL, PRIORITY, 0, 0);
