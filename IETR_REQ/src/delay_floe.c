#include "stm32g4xx.h"

// delay permet de boucler sur un "nop" en assembleur t fois.
// paramètres: un unsigned int
void delay(volatile uint32_t t)
{
    while(t--)
    {
        asm volatile ("nop");
    }
}
