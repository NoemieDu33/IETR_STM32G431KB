#include "lcd_floe.h"
#include "stm32g431xx.h"
#include <string.h>
#include "delay_floe.h"


/*
D7 D6 D5 D4 sont les data pins.

E est l'enable pin: lorsqu'un nombre est indiqué avec les data pins, 
            passer E à 1 va enregistrer le nombre et l'interpréter

RS est passé à 1 au moment d'écrire des lettres, et à 0 pour config le LCD.
*/

/*
Table d'instructions:
[ORDRE : D7 - D6 - D5 - D4]
Une instruction (sur 8 bits) est une paire de commandes sur 4 bits.

Par exemple, 
- CLEAR DISPLAY est l'instruction 0x1, traduite comme suivant:
0000 -> ENABLE -> 0001 -> ENABLE
- 4 BIT-MODE est l'instruction 0x20, traduite comme suivant:
0010 -> ENABLE -> 0000 -> ENABLE

Quand RS est activé (1), se référer à une table ASCII.
________________________________________________________________________________

0x03 ---> Reset Command, à faire 3 fois d'affilée pour reset le LCD (voir la fonction hard_reset)

0x02 ---> Passage en mode 4 bits (car le LCD peut aussi être utilisé avec 8 data pins)
          (Fortement conseillé)

_____________________________________UNE FOIS QUE LE DISPLAY EST EN MODE 4 BITS

0x1 ---> Nettoyer l'écran
0x2 ---> Ramener le curseur en position 0
0x8 ---> Eteindre l'écran (uniquement l'affichage, pas de reset)
0xC ---> Allumer l'écran, sans curseur, sans clignotement du curseur
0xE ---> Allumer l'écran, avec curseur, sans clignotement du curseur
0xF ---> Allumer l'écran, avec curseur, avec clignotement du curseur
0xC0 ---> Sauter une ligne
0x18 ---> Scroll de droite à gauche, e curseur ne scroll pas (reste au même endroit visuellement)

RS à 0x1 --> Mode d'écriture sur la RAM (donc écriture de caractères)
*/


// init_pins_lcd initialise les pins qui feront marcher l'écran. Il y en a 6 notables. 
// La fonction doit être appelée en première, avant init_16x2.
void init_pins_lcd(void){
    /*
    MODER prend deux bits de paramètre:
    00 = input mode,
    01 = output mode,
    10 = alternate function mode,
    11 = analog mode / reset state
    */

    GPIOB -> MODER &= ~(0x3 << 2*RS); 
    GPIOB -> MODER |= (0x1 << 2*RS); // On met les pins en OUTPUT (0x1, sur 2 bits donc 2*PIN)
    GPIOB -> OTYPER &= ~(0x1 << RS); // 0x0 = Push-Pull, 0x1 = Open-drain. Ici, push-pull

    /*
    PUPDR prend deux bits de paramètre:
    00 = Rien,
    01 = Pull-Up,
    10 = Pull-Down,
    11 = Réservé
    */

    GPIOB -> PUPDR &= ~(0x3 << 2*RS); 
    GPIOB -> PUPDR |= (0x1 << 2*RS); // Pull-Up


    // On ne se sert pas du registre RW dans la config actuelle. Il est relié au GND.

    // GPIOB -> MODER &= ~(0x3 << 2*RW); 
    // GPIOB -> MODER |= (0x1 << 2*RW);
    // GPIOB -> OTYPER &= ~(0x1 << RW);
    // GPIOB -> PUPDR &= ~(0x3 << 2*RW);
    // GPIOB -> PUPDR |= (0x1 << 2*RW);

    GPIOB -> MODER &= ~(0x3 << 2*D7); 
    GPIOB -> MODER |= (0x1 << 2*D7);

    GPIOB -> OTYPER &= ~(0x1 << D7);

    GPIOB -> PUPDR &= ~(0x3 << 2*D7);
    GPIOB -> PUPDR |= (0x1 << 2*D7);

    GPIOB -> MODER &= ~(0x3 << 2*D6); 
    GPIOB -> MODER |= (0x1 << 2*D6);

    GPIOB -> OTYPER &= ~(0x1 << D6);

    GPIOB -> PUPDR &= ~(0x3 << 2*D6);
    GPIOB -> PUPDR |= (0x1 << 2*D6);

    GPIOB -> MODER &= ~(0x3 << 2*D5); 
    GPIOB -> MODER |= (0x1 << 2*D5);

    GPIOB -> OTYPER &= ~(0x1 << D5);

    GPIOB -> PUPDR &= ~(0x3 << 2*D5);
    GPIOB -> PUPDR |= (0x1 << 2*D5);

    GPIOB -> MODER &= ~(0x3 << 2*D4); 
    GPIOB -> MODER |= (0x1 << 2*D4);

    GPIOB -> OTYPER &= ~(0x1 << D4);

    GPIOB -> PUPDR &= ~(0x3 << 2*D4);
    GPIOB -> PUPDR |= (0x1 << 2*D4);

    GPIOA -> MODER &= ~(0x3 << 2*E); //Attention, E est sur GPIOA contrairement au reste ! 
    GPIOA -> MODER |= (0x1 << 2*E);

    GPIOA -> OTYPER &= ~(0x1 << E);

    GPIOA -> PUPDR &= ~(0x3 << 2*E);
    GPIOA -> PUPDR |= (0x1 << 2*E);
}

// send_1 envoie un état HAUT sur la broche désignée.
// Paramètres: un GPIO_TypeDef* (GPIOA ou GPIOB), un int pour le numéro du pin
void send_1(GPIO_TypeDef* gpio, int pin){
    gpio -> ODR |= (0x1 << pin); // ODR est le registre de sorties GPIO. Ici, le pin est mis à 1
}

// send_0 envoie un état BAS sur la broche désignée.
// Paramètres: un GPIO_TypeDef* (GPIOA ou GPIOB), un int pour le numéro du pin
void send_0(GPIO_TypeDef* gpio, int pin){
    gpio -> ODR &= ~(0x1 << pin);
}

// data_1 envoie un état HAUT à toutes les broches DATA du LCD.
void data_1(void){ // Les data-pins sont D7, D6, D5, D4, différents des pins E et RS.
    GPIOB->ODR |= (0x1 << D7);
    GPIOB->ODR |= (0x1 << D6);
    GPIOB->ODR |= (0x1 << D5);
    GPIOB->ODR |= (0x1 << D4);
}

// data_0 envoie un état BAS à toutes les broches DATA du LCD.
void data_0(void){
    GPIOB->ODR &= ~(0x1 << D7);
    GPIOB->ODR &= ~(0x1 << D6);
    GPIOB->ODR &= ~(0x1 << D5);
    GPIOB->ODR &= ~(0x1 << D4);
}

// reg_command (register command) crée une impulsion sur a broche ENABLE.
// Une fois que les data pins sont réglés, il doit y avoir une impulsion sur la broche ENABLE pour valider une commande.
void reg_command(void){ // Les delays sont importants, le "temps de réaction" du LCD est dans l'ordre des ms.
    delay(DELAY);
    send_1(GPIOA, E);
    delay(DELAY);
    send_0(GPIOA, E);
    delay(DELAY);
    data_0();
    delay(DELAY);
}

// send_command_4b règle les data pins en fonction de la commande passée.
// Paramètres: un unsigned char pour la commande
void send_command_4b(uint8_t cmd){
    if (cmd & 0x01){
        send_1(GPIOB, D4);
    }
    if (cmd & 0x02){
        send_1(GPIOB, D5);
    }
    if (cmd & 0x04){
        send_1(GPIOB, D6);
    }
    if (cmd & 0x08){
        send_1(GPIOB, D7);
    }
    reg_command();
}

// send_command_8b divise une commande sur un octet en deux paires de commandes de 4 bits, et les envoie aux data pins.
// Paramètres: un unsigned char pour la commande
void send_command_8b(uint8_t cmd){
    send_command_4b(cmd >> 4);
    send_command_4b(cmd & 0x0F);
}

// hard_reset réinitialise le LCD en envoyant trois fois la commande 0x03.
void hard_reset(void){
    send_command_4b(0x03);
    send_command_4b(0x03);
    send_command_4b(0x03);
}

// init_16x2 envoie au LCD les commandes d'initialisation pour fonctionner.
void init_16x2(void){
    send_command_4b(0x08); // 1000, pin D7 - éteindre l'écran
    hard_reset();
    send_command_4b(0x02); // 0010, pin D5 - passage en 4 bits

    send_command_8b(0x01); // 0000 0001, pin D4 - clear display
    send_command_8b(0x02); // 0000 0010, pin D5 - return home
    send_command_8b(0xF); // 0000 1100, pins D7, D6 - display on, cursor and blink

}

// newline fait sauter une ligne au LCD.
void newline(void){
    send_command_8b(0xC0);
}

// scroll_left fait défiler l'écran d'un cran vers la gauche.
void scroll_left(void){
    for (int i=0; i<16; i++){
        send_command_8b(0x18);
    }
}

// send_letter écrit un caractère sur le LCD.
// Paramètres: un char
void send_letter(char c){
    int buf[8];
    int asciival = c;
    for (int i = 0; i < 8; i++) {
        buf[i] = !!((asciival << i) & 0x80); // Conversion décimal -> binaire dans un tableau.
    }                                        // Par exemple, 27 -> [0, 0, 0, 1, 1, 0, 1, 1]

    send_1(GPIOB, RS); // Pour écrire des caractères

    if (buf[0]){send_1(GPIOB, D7);} // Lors des commandes de 8 bits,
    if (buf[1]){send_1(GPIOB, D6);} // en 4 bit mode,
    if (buf[2]){send_1(GPIOB, D5);} // on flashe d'abord les MSB
    if (buf[3]){send_1(GPIOB, D4);} // (exemple: si on a 0x1B, ici on flashe 0x1)
    reg_command();


    if (buf[4]){send_1(GPIOB, D7);} // et ensuite les LSB
    if (buf[5]){send_1(GPIOB, D6);} // (exemple: si on a 0x1B, ici on flashe 0xB)
    if (buf[6]){send_1(GPIOB, D5);}
    if (buf[7]){send_1(GPIOB, D4);}
    reg_command();

    send_0(GPIOB, RS); // Fin de transmission d'un caractère

}

// send_text appelle send_letter pour chaque caractère d'un string.
// Paramètres: un tableau de char
void send_text(char* s){
    for (int i=0; i<strlen(s); i++){
        send_letter(s[i]);
    }
}

// clear_screen nettoie l'écran et remet le curseur au premier emplacement.
void clear_screen(void){
    send_command_8b(0x01); // 0000 0001, pin D4 - clear display
    send_command_8b(0x02); // 0000 0010, pin D5 - return home
    send_command_8b(0xF); // 0000 1100, pins D7, D6 - display on, cursor, blink
}