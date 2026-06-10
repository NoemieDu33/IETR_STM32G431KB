#include "stm32g431xx.h"
#include <stm32g4xx.h>
#include <string.h>

#include "lcd_floe.h"
#include "led_floe.h"
#include "uart_floe.h"
#include "delay_floe.h"

#define RST_BUTTON 10 // GPIOA 
#define REQ1_BUTTON 9 // GPIOA
#define REQ2_BUTTON 0 // GPIOA
#define REQ3_BUTTON 1 // GPIOA


// Faut que les caractères soient ASCII

#define NEWLINE_TAG '-' // Caractère qui cause un saut de ligne. Par défaut '-'
#define END_RX_TAG '\n' // Caractère qui coupe la réception d'un msg. Par défaut '\n'


volatile int flag_req = 0;

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
    GPIOA->MODER &= ~(0x3 << 2*RST_BUTTON); // Bouton de reset de l'écran
    GPIOA->MODER &= ~(0x3 << 2*REQ1_BUTTON); // Bouton de gauche
    GPIOA->MODER &= ~(0x3 << 2*REQ2_BUTTON); // Bouton du milieu
    GPIOA->MODER &= ~(0x3 << 2*REQ3_BUTTON); // Bouton de droite
    
    // On met tout les boutons en pull-up (0x1, sur 2 bits donc 2*ADDR)
    GPIOA->PUPDR &= ~(0x3 << 2*RST_BUTTON);
    GPIOA->PUPDR |= (0x1 << 2*RST_BUTTON);
    GPIOA->PUPDR &= ~(0x3 << 2*REQ1_BUTTON);
    GPIOA->PUPDR |= (0x1 << 2*REQ1_BUTTON);
    GPIOA->PUPDR &= ~(0x3 << 2*REQ2_BUTTON);
    GPIOA->PUPDR |= (0x1 << 2*REQ2_BUTTON);
    GPIOA->PUPDR &= ~(0x3 << 2*REQ3_BUTTON);
    GPIOA->PUPDR |= (0x1 << 2*REQ3_BUTTON);

    SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI10 | // EXTI du pin A10
                            SYSCFG_EXTICR3_EXTI9 | // EXTI du pin A9
                            SYSCFG_EXTICR1_EXTI0 | // EXTI du pin A0
                            SYSCFG_EXTICR1_EXTI1); // EXTI du pin A1


    EXTI->IMR1 |= EXTI_IMR1_IM10; // Ne pas masquer la ligne EXTI10 (ça revient à activer l'interruption)
    EXTI->FTSR1 |= EXTI_FTSR1_FT10; // Activer Front descendant
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT10; // Désactiver front montant

    EXTI->IMR1 |= EXTI_IMR1_IM9; // Et on fait la même pour EXTI9, 1 et 0
    EXTI->FTSR1 |= EXTI_FTSR1_FT9;
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT9;

    EXTI->IMR1 |= EXTI_IMR1_IM0;
    EXTI->FTSR1 |= EXTI_FTSR1_FT0;
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT0;

    EXTI->IMR1 |= EXTI_IMR1_IM1;
    EXTI->FTSR1 |= EXTI_FTSR1_FT1;
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT1;

    // NVIC est le contrôleur d'interruption global. Il faut "déclarer" les interruptions
    // EXTI0 et EXTI1 sont uniques
    // EXTI9 appartient à un bus : EXTI9_5
    // EXTI10 appartient à un bus : EXTI15_10
    
    NVIC_SetPriority(EXTI15_10_IRQn, 2);
    NVIC_SetPriority(EXTI9_5_IRQn, 2);
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_SetPriority(EXTI1_IRQn, 2);

    // Et on active les interruptions dans le NVIC

    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);

}

// Les noms des fonctions d'interruption sont stricts et ne doivent pas être modifié

// EXTI15_10_IRQHandler gère les interruptions sur le Pin PA10.
void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF10) { // Le if permet de vérifier si c'est le bon pin qui a trigger l'interruption
        
        clear_screen(); // Contenu de l'interruption ici. 

        EXTI->PR1 = EXTI_PR1_PIF10; // Pour reset l'interruption il faut mettre le pin à 1
    }
}

// EXTI9_5_IRQHandler gère les interruptions sur le Pin PA9.
void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF9) {
        
        clear_screen();
        send_text("REQ 1");
        send_string("REQ 1\n");
        delay(100000);
        clear_screen();
        flag_req = 1;

        EXTI->PR1 = EXTI_PR1_PIF9;
    }
}

// EXTI0_IRQHandler gère les interruptions sur le Pin PA0.
void EXTI0_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF0) {
        
        clear_screen();
        send_text("REQ 2");
        send_string("REQ 2\n");
        delay(100000);
        clear_screen();
        flag_req = 1;

        EXTI->PR1 = EXTI_PR1_PIF0;
    }
}

// EXTI1_IRQHandler gère les interruptions sur le Pin PA1.
void EXTI1_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF1) {
        
        clear_screen();
        send_text("REQ 3");
        send_string("REQ 3\n");
        delay(100000);
        clear_screen();
        flag_req = 1;

        EXTI->PR1 = EXTI_PR1_PIF1;
    }
}

int main(void){
    init_gpios(); // Il faut que le premier module initialisé active GPIOA et GPIOB
    init_uart();
    init_led();
    init_irq();
    __enable_irq();

    init_pins_lcd();
    delay(DELAY); // Les delays sont importants pour que le 16x2 enregistre les infos (parfois il a besoin de qlq ms)
    init_16x2();
    delay(DELAY);
    
    char buf[64];
    int i=0;

    send_text("ALL OK");

    while(1){ // END_RX_TAG indique une fin de trame sur le LCD, et NEWLINE_TAG indique un saut de ligne. Voir les #define
        if (flag_req==1){
            char e = '\0';
            e = recv_c();
                if (!(e==END_RX_TAG)){
                    LED_ON();
                    if (!(e==NEWLINE_TAG)){
                        send_letter(e);
                        delay(DELAY);
                    } else {
                        newline();
                        delay(DELAY);
                    }
                } else {
                    flag_req = 0;
                    LED_OFF();
                }
        }


    //     else {
    //         i++;
    //         if (i==1){
    //             newline();
    //         } else if (i==2){
    //             RED_LED_ON();
    //             delay(2000000);
    //             clear_screen();
    //             RED_LED_OFF();
    //             i=0;
    //         }

    //         }

        } 



    }
