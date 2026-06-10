#include "stm32g431xx.h"
#include <stm32g4xx.h>

#define LED_PIN 8 // C'est built-in, mais physiquement GPIO B

void init_led(void);
void LED_ON(void);
void LED_OFF(void);