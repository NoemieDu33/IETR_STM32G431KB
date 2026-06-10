#include "stm32g431xx.h"
#include <stm32g4xx.h>
#include <string.h>
#include <stdio.h>
#include "uart_floe.h"
#include "delay_floe.h"

// init_uart : initialise un périphérique UART / USART pour ensuite l'utiliser.
// Doit être appelé en premier (avant l'utilisation de send_c et recv_c)
void init_uart(void){

    // Premièrement il faut activer les horloges des modules qui nous intéressent.

    RCC -> APB1ENR1 |= RCC_APB1ENR1_USART2EN; // On active l'USART 2

    /*
    MODER prend deux bits de paramètre:
    00 = input mode,
    01 = output mode,
    10 = alternate function mode,
    11 = analog mode / reset state
    */

    GPIOA -> MODER &= ~(0x3 << (2*TX_PIN)); // On passe tout en reset state sauf MODER2 (2*PIN car c'est des registres de 2 bits)
    GPIOA -> MODER |= (0x2 << (2*TX_PIN)); // Pour ensuite passer MODER2 en alternate

    GPIOA -> MODER &= ~(0x3 << (2*RX_PIN)); // On passe tout en reset state sauf MODER15 (2*PIN car c'est des registres de 2 bits)
    GPIOA -> MODER |= (0x2 << (2*RX_PIN)); // Pour ensuite passer MODER15 en alternate


    // AFR[0] c'est AFRL dans la doc, ça couvre les pins de 0 à 7
    // AFR[1] c'est AFRH, ça couvre les pins de 8 à 15
    
    /*
    AFRL et AFRH permettent de sélectionner les fonctions alternate pour les pins.
    Par exemple, celui qui nous intéresse c'est AF7 (donc affecter 7 sur notre pin dans le registre AFR)
    Car AF7 représente l'USART2.
    */

    GPIOA -> AFR[0]  &= ~(0xF << (4*TX_PIN)); // On met le pin TX à 0. 
    GPIOA -> AFR[0] |=  (7 << (4*TX_PIN)); // On met AF7 pour notre pin TX. (4*PIN car c'est des registres de 4 bits)
    GPIOA -> AFR[1]  &= ~(0xF << (4*(RX_PIN-8))); // On met le pin RX à 0. RX_PIN-8 car dans AFRH, le bit n couvre la broche n+8
    GPIOA -> AFR[1] |=  (0x7 << (4*(RX_PIN-8))); // On met AF7 pour notre pin RX. (4*PIN car c'est des registres de 4 bits)

    USART2 -> BRR = 16000000 / BAUDRATE; // On affecte BRR à notre SYSCLK divisée par la baudrate souhaitée.

    /*
    Les bits qui nous intéressent sont:
    - RE (bit 2, Reciever Enable)
    - TE (bit 3, Transmitter Enable)
    - UE (bit 0, USART Enable)
    */

    USART2 -> CR1 = USART_CR1_TE;
    USART2 -> CR1 |= USART_CR1_RE;
    USART2 -> CR1 |= USART_CR1_UE;

    // Deux while pour forcer l'attente le temps que ça s'initialise

    while (!(USART2->ISR & USART_ISR_REACK)); // REACK passe à 1 quand la réception est correctement activée
    while (!(USART2->ISR & USART_ISR_TEACK)); // TEACK passe à 1 quand la transmission est correctement activée

}

// send_c transmet un caractère c passé en paramètre au transmit data register du périphérique UART.
// paramètres : un char
void send_c(char c){
    while (!(USART2->ISR & USART_ISR_TXE_TXFNF)); // TXE = Transmit Data Register Empty (donc = 1 quand on peut transmettre)
    USART2 -> TDR = c; // Transmit Data Register (Les données écrites dedans sont transmises)
}

// recv_c récupère le contenu du recieved data register.
// renvoie: un char
char recv_c(){
    while (!(USART2->ISR & USART_ISR_RXNE_RXFNE)); // RXNE = Recieve Data Register Not Empty (donc = 1 quand données reçues)
    return (char)USART2->RDR; // RDR = Recieved Data Register (Si lu, alors RXNE repasse à 0)
}

// send_string appelle send_c sur chaque caractère d'un string.
// paramètres: un tableau de char
void send_string(char* s){
    int length = strlen(s);
    for (int i=0; i<length; i++){
        send_c(s[i]);
        delay(100000);
    }
}

