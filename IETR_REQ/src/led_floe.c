#include "stm32g431xx.h"
#include <stm32g4xx.h>
#include "led_floe.h"

// init_led initialise la broche physique reliée à la LED (PB8). Doit être appelé avant LED_ON et LED_OFF.
void init_led(void){
    GPIOB -> MODER &= ~(0x3 << 2*LED_PIN); 
    GPIOB -> MODER |= (0x1 << 2*LED_PIN); // MODER8 à 01 (OUTPUT)
}

// LED_OFF éteint la LED.
void LED_OFF(void){
    GPIOB->ODR &= ~(1 << LED_PIN); // ODR = Output Data Register, LED allumée si ODR8 = 1, éteinte si ODR8 = 0
}

// LED_ON allume la LED.
void LED_ON(void){
    GPIOB->ODR |= (1 << LED_PIN);
}