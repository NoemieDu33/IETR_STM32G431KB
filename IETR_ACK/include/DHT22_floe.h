#include "stm32g4xx.h"

#define DHT_PIN 1 //GPIOA 

typedef struct {
    uint8_t humidity_int;
    uint8_t humidity_dec;
    uint8_t temperature_int;
    uint8_t temperature_dec;
} DHT22_Data_t;

void setup_HSI_CLK(void);

void init_TIMER(void);

void delay_tim(uint16_t us);

int wait_pin_state(uint8_t state, uint16_t timeout_us);

void init_DHT22(void);

void switch_input_DHT22(void);
void switch_input_DHT22(void);

int get_value_DHT22(DHT22_Data_t* data);

