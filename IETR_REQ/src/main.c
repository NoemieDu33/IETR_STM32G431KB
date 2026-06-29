#include "stm32g431xx.h"
#include <stdlib.h>
#include <stm32g4xx.h>
#include <string.h>

// #include "lcd_floe.h"
#include "led_floe.h"
#include "uart_floe.h"
#include "delay_floe.h"
#include "ssd1306_floe.h"

#define REQ1_BUTTON 0 // GPIOA
#define REQ2_BUTTON 1 // GPIOA
#define REQ3_BUTTON 3 // GPIOA


// Faut que les caractères soient ASCII



volatile int flag_req = 0;
volatile int requestnumber = 0;
volatile int flag_setup_graph = 0;

// init_gpios active les horloges des GPIOA et GPIOB pour les différents modules utilisés. Doit être lancé en premier.
void init_gpios(void){
    RCC -> AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC -> AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
}

// init_irq permet d'initialiser les interruptions externes reliées aux appuis de boutons (PA0, PA1, PA9, PA10).
void init_irq(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // On active le module SYSCFG
    // SYSCFG nous permettra d'ensuite setup les interruptions externes

    /*
    MODER prend deux bits de paramètre:
    00 = input mode,
    01 = output mode,
    10 = alternate function mode,
    11 = analog mode / reset state
    */

    // On passe tout les boutons en input (0x0, sur 2 bits donc 2*ADDR)
    GPIOA->MODER &= ~(0x3 << 2*REQ1_BUTTON); // Bouton de gauche
    // GPIOA->MODER &= ~(0x3 << 2*REQ2_BUTTON); // Bouton du milieu
    // GPIOA->MODER &= ~(0x3 << 2*REQ3_BUTTON); // Bouton de droite
    
    // On met tout les boutons en pull-up (0x1, sur 2 bits donc 2*ADDR)

    GPIOA->PUPDR &= ~(0x3 << 2*REQ1_BUTTON);
    GPIOA->PUPDR |= (0x1 << 2*REQ1_BUTTON);
    // GPIOA->PUPDR &= ~(0x3 << 2*REQ2_BUTTON);
    // GPIOA->PUPDR |= (0x1 << 2*REQ2_BUTTON);
    // GPIOA->PUPDR &= ~(0x3 << 2*REQ3_BUTTON);
    // GPIOA->PUPDR |= (0x1 << 2*REQ3_BUTTON);

    SYSCFG->EXTICR[0] &= ~(0xFUL << (0 * 4)); // Nettoie EXTI0 (Bits 0..3) -> PA0
    // SYSCFG->EXTICR[0] &= ~(0xFUL << (1 * 4)); // Nettoie EXTI1 (Bits 4..7) -> PA1
    // SYSCFG->EXTICR[0] &= ~(0xFUL << (3 * 4)); // Nettoie EXTI3 (Bits 12..15) -> PA3


    EXTI->IMR1 |= EXTI_IMR1_IM0;
    EXTI->FTSR1 |= EXTI_FTSR1_FT0;
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT0;

    // EXTI->IMR1 |= EXTI_IMR1_IM1;
    // EXTI->FTSR1 |= EXTI_FTSR1_FT1;
    // EXTI->RTSR1 &= ~EXTI_RTSR1_RT1;

    // EXTI->IMR1 |= EXTI_IMR1_IM3;
    // EXTI->FTSR1 |= EXTI_FTSR1_FT3;
    // EXTI->RTSR1 &= ~EXTI_RTSR1_RT3;

    NVIC_SetPriority(EXTI0_IRQn, 2);
    // NVIC_SetPriority(EXTI1_IRQn, 2);
    // NVIC_SetPriority(EXTI3_IRQn, 2);

    // Et on active les interruptions dans le NVIC

    NVIC_EnableIRQ(EXTI0_IRQn);
    // NVIC_EnableIRQ(EXTI1_IRQn);
    // NVIC_EnableIRQ(EXTI3_IRQn);

}


// EXTI0_IRQHandler gère les interruptions sur le Pin PA0.
void EXTI0_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF0) {
        
            flag_req = 1;
            requestnumber = 3;

        EXTI->PR1 = EXTI_PR1_PIF0;
    }
}

// void EXTI1_IRQHandler(void) {
//     if (EXTI->PR1 & EXTI_PR1_PIF1) {
        
//             LED_ON();
//             clear_screen_o();
//             update_screen();
//             send_string("REQ 2\n");
//             LED_OFF();
//             flag_req = 1;

//         EXTI->PR1 = EXTI_PR1_PIF1;
//     }
// }

// void EXTI3_IRQHandler(void) {
//     if (EXTI->PR1 & EXTI_PR1_PIF3) {
        
//             LED_ON();
//             clear_screen_o();
//             draw_template_o();
//             update_screen();
//             send_string("REQ 3\n");
//             LED_OFF();
//             flag_req = 1;

//         EXTI->PR1 = EXTI_PR1_PIF3;
//     }
// }

int main(void){
    init_gpios();
    delay(100000);
    init_led();
    init_uart();
    init_oled();
    draw_welcome_o();
    update_screen();
    init_irq();
    __enable_irq();

    int i=8;
    int j=8;
    char buf[256];
    int k = 0;
    int adcval;
    char* cmval[8];

    while(1){
        LED_OFF();
        if (flag_req == 1){
            if (requestnumber==3){
                send_string("REQ 3\n");
                if (!(flag_setup_graph)){
                    i = 100;
                    j = 32;
                    clear_screen_o();
                    draw_template_o();
                    flag_setup_graph = 1;
                }

                char e = recv_c();
                
                if (e != '\0') { // Si on a bien reçu un caractère
                    LED_ON();
                    if (e != '\n') {
                        if (k < 255) {
                            buf[k++] = e;
                        }
                    } else {
                        // Fin de chaîne atteinte ('\n' ou '-')
                        buf[k] = '\0'; // Finir la chaîne proprement pour printf/send_text
                        adcval = atoi(buf);
                        k = 0;
                        adcval = (int)(61 * (adcval/4095));
                        graph_o(adcval);
                        update_screen(); // Rafraîchissement flash de l'écran !
                        delay(10000000);
                    }
                }


            } else {
                LED_OFF();
                char e = recv_c();
                
                if (e != '\0') { // Si on a bien reçu un caractère
                    if (e != '\n') {
                        if (k < 255) {
                            buf[k++] = e;
                        }
                    } else {
                        // Fin de chaîne atteinte ('\n' ou '-')
                        buf[k] = '\0'; // Finir la chaîne proprement pour printf/send_text
                        
                        clear_screen_o();
                        LED_ON();
                        send_text_o(buf, 8, 8);
                        update_screen(); // Rafraîchissement flash de l'écran !
                        
                        k = 0; // Reset de l'index du buffer 
                        flag_req = 0; // On a reçu toute la réponse, on baisse le drapeau
                    }
                }
            }
        }  
    }
}