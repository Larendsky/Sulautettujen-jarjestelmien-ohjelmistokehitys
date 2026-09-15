// *****************************************************
// This program shows how to use button 0 with interrupt
// with nRF5340 Audio boards
// Modified from the zephyr/button sample

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)

static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static struct gpio_callback button_1_data;
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static struct gpio_callback button_2_data;

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

int init_button(void);
int init_led(void);
volatile int led_state = 0;
volatile int previous_state = 0; //muistaa aiemman tilan pausea varten

// Button interrupt handler
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button pressed\n");

	if (led_state !=4){
		previous_state = led_state; //tallenna nykyinen tila
		led_state = 4; //muuta tilaksi pause
		printk("pause ON\n");
	} else {
		led_state = previous_state; //jos tila on jo pause palauta tallennettu tila
		printk("Pause OFF\n");
	}
}

// Main program
int main(void) //tavoittelen 2 pistettä
{

	int ret = init_button();
	if (ret < 0) {
		return 0;
	}

init_led();

	while (1) {
		k_msleep(10); // sleep 10ms
	}

	return 0;
}

// Button initialization
int init_button() {

	int ret;
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);
	printk("Set up button 0 ok\n");
	
	return 0;
}
	// Initialize leds
int init_led(void) {
    int ret;

    // Punainen ledi alustus ja sammutus (0)
    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) return ret;
    gpio_pin_set_dt(&red, 0);

    // Vihreä ledi alustus ja sammutus (0)
    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) return ret;
    gpio_pin_set_dt(&green, 0);

    // Sininen ledi alustus ja sammutus (0)
    ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) return ret;
    gpio_pin_set_dt(&blue, 0);

    printk("Leds initialized ok\n");
    return 0;
}

#define STACKSIZE 500
#define PRIORITY 5

//1 punainen taski
void red_led_task(void *a,void *b,void *c) {
	while (true){
		if (led_state == 0){
			gpio_pin_set_dt(&red, 1); //punainen syttyy
			k_msleep(1000); //sekuntti
			gpio_pin_set_dt(&red, 0); //sammuta
			if (led_state != 4){ //tarkistaa onko pausella. Väri vaihtuu vain jos ei ole
			led_state = 1; 
		}
		}else{
			k_msleep(10);
		}
	}
}
K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

//2 keltainen taski
void yellow_led_task(void *a,void *b,void *c) {
	while (true){
		if (led_state == 1){
			gpio_pin_set_dt(&red, 1);
			gpio_pin_set_dt(&green, 1);
			k_msleep(1000);
			gpio_pin_set_dt(&red, 0);
			gpio_pin_set_dt(&green, 0);
			if (led_state != 4){ //tarkistaa onko pausella. Väri vaihtuu vain jos ei ole
			led_state = 2;
			}
		} else{
			k_msleep(10);
		}
	}
}
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

//3 vihrea taski
void green_led_task(void *a, void *b, void *c){
	while (true){
		if (led_state ==2){
			gpio_pin_set_dt(&green, 1); //sytytä vihree
			k_msleep(1000);
			gpio_pin_set_dt(&green, 0); //sammuta vihree
			if (led_state != 4){ //tarkistaa onko pausella. Väri vaihtuu vain jos ei ole
			led_state = 0; //vaihda tila takaisin punaseen
			}
		} else {
			k_msleep(10);
		}
	}
}
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
