#include "DHT22_floe.h"
#include "stm32g431xx.h"

// guide suivi : https://embetronicx.com/tutorials/microcontrollers/stm32/simple-stm32-timer-tutorial-bare-metal-with-registers/


// setup_HSI_CLK initialise l'horloge HSI (high speed internal) qu'on utilisera pour créer un TIMER. Doit être appelé en premier.
// A noter que la SYSCLK du MCU devient le HSI (16MHz) après le setup.
void setup_HSI_CLK(void){
    // On utilise l'horloge HSI comme horloge principale du MCU (SYSCLK) à 16HMz pour construire un TIMER à la microseconde près.
    // C'est important pour nos mesures sur le DHT22.
    if ((RCC->CR & RCC_CR_HSIRDY) == 0){ // HSIRDY = Horloge HSI activée (donc à 0 si elle n'est pas setup)
        RCC -> CR |= RCC_CR_HSION; // HSION = On active l'horloge HSI
        while((RCC->CR & RCC_CR_HSIRDY) == 0); // Puis on laisse l'horloge s'initialiser.
    }

    // CFGR permet de modifier les fréquences vers les diff. bus (Divisions, etc).
    // On ne met aucune division sur nos bus AHB, APB1 et APB2.

    RCC -> CFGR |= RCC_CFGR_HPRE_DIV1; // AHB
    RCC -> CFGR |= RCC_CFGR_PPRE1_DIV1; // APB1
    RCC -> CFGR |= RCC_CFGR_PPRE2_DIV1; // APB2

    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW)); 
    RCC->CFGR |= RCC_CFGR_SW_HSI; // SW = switch, ici on met la HSI comme SYSCLK
    
    FLASH->ACR = FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_0WS; // Quand une CLK va trop vite (genre 170MHz) il faut
                                                                        // mettre des wait state. Pas besoin ici.
                                                                        // Also DCEN et ICEN sont des caches pour accélérer les lectures.

    RCC->CR &= ~RCC_CR_HSEON; // On s'assure que HSE est désactivé (différent de HSI. I = internal, E = external)
}

// init_TIMER crée un timer à partir du HSI initialisé avant. Doit être appelé juste après setup_HSI_CLK.
void init_TIMER(void){
    RCC -> APB1ENR1 |= RCC_APB1ENR1_TIM3EN; // TIMER 3 Enable (similaire aux GPIOAEN, GPIOBEN, USART2EN...)
    TIM3 -> PSC = 15; // F_timer = F_clk (16MHz) / (PSC+1). Si on veut un timer à 1MHz (donc incrément chaque 1us), on met PSC à 15
    TIM3 -> ARR = 0xFFFF; // 65535. Automatic reload définit le temps que prend un cycle avant de revenir à 0. Ici, 65.536ms 
    TIM3 -> EGR |= TIM_EGR_UG; // Permet d'update manuellement le PSC
    TIM3 -> CR1 |= TIM_CR1_CEN; // Permet de démarrer le TIMER
}


// delay_tim crée des délais à la microseconde près.
// Paramètres: un unsigned short
void delay_tim( uint16_t us ){ // Génération de délais
volatile uint16_t start = TIM3->CNT; // On stocke la valeur du compteur à l'instant START 
    while ((uint16_t)((volatile uint16_t)TIM3->CNT - start) < us){} // On attend us microsecondes.
}

// wait_pin_state mesure la durée d'un délai sur une broche.
// Paramètres: un unsigned char pour l'état, un unsigned short pour le timeout
// Retourne: un int (0 = ALL OK, -1 = TIMEOUT ERROR, -2 = CHECKSUM ERROR)
int wait_pin_state(uint8_t state, uint16_t timeout_us){ // Mesure de délais
    volatile uint16_t start = TIM3->CNT; // On stocke la valeur du compteur à l'instant START

    if (state) {
        while (!(GPIOA->IDR & (1 << DHT_PIN))) { 
            if ((uint16_t)((volatile uint16_t)TIM3->CNT - start) > timeout_us) return -1; 
        }
    } else {
        while (GPIOA->IDR & (1 << DHT_PIN)) {
            if ((uint16_t)((volatile uint16_t)TIM3->CNT - start) > timeout_us) return -1;
        }
    }
    return 0;
}

/*
Le DHT22 fonctionne en one-wire: toutes les données se font par un seul fil.
Au début, on le met en output et on lui envoie un signal bas pendant 18ms.
Ensuite, on le met en input et on attend qu'il envoie ses signaux.
Il communiquera un ACK (0 -> 1 -> 0), puis il communiquera 40 bits, l'état (0 ou 1) est défini par la durée.

Chaque bit est communiqué de la façon suivante:
LOW pendant 50us --> HIGH (la durée du high détermine la donnée)
Un 0 durera environ 26us, un 1 durera environ 70us.
Il envoie 5 octets.
*/


// init_DHT22 initialise la broche du DHT22. Doit être appelé avant get_value_DHT22.
void init_DHT22(void){
    GPIOA -> PUPDR &= ~(0x3 << 2*DHT_PIN); 
    GPIOA -> PUPDR |= (0x1 << 2*DHT_PIN); // Pull-Up
    GPIOA -> MODER &= ~(0x3 << 2*DHT_PIN);
    GPIOA -> MODER |= (0x1 << 2*DHT_PIN); //au début on le met en output
    GPIOA -> OTYPER |= (0x1 << DHT_PIN); // 1 = open drain, 0 = push-pull 

}

// switch_output_DHT22 passe le pin du DHT22 en mode sortie.
void switch_output_DHT22(void){
    GPIOA -> MODER &= ~(0x3 << 2*DHT_PIN); 
    GPIOA -> MODER |= (0x1 << 2*DHT_PIN);
}

// switch_input_DHT22 passe le pin du DHT22 en mode entrée.
void switch_input_DHT22(void){
    GPIOA -> MODER &= ~(0x3 << 2*DHT_PIN); 
}

// get_value_DHT22 récupère les 5 octets de valeurs du DHT22.
// Paramètres: Un struct à 4 éléments (temperature entier, temperature decimal, humidite entier, humidite decimal)
// Renvoie: Un int (0 = ALL OK, -1 = TIMEOUT ERROR, -2 = CHECKSUM ERROR)
int get_value_DHT22(DHT22_Data_t* data){

    uint8_t raw_bytes[5] = {0};

    switch_output_DHT22(); // Le pin en output

    GPIOA -> ODR &= ~(0x1 << DHT_PIN); // Signal bas pendant 18ms
    delay_tim(18000); 

    GPIOA -> ODR |= (0x1 << DHT_PIN); // On le repasse en signal haut
    delay_tim(30); // Toute mini attente pour que le DHT process l'info

    switch_input_DHT22(); // On le passe en input

    if (wait_pin_state(0, 100) != 0) return -1; // Avant les 40 bits, il envoie son ACK (0, 1, 0)
    if (wait_pin_state(1, 100) != 0) return -1; // On lève un timeout s'il met + de 100us par bit
    if (wait_pin_state(0, 100) != 0) return -1;

    for (int i = 0; i < 40; i++) // Lecture des 5 octets
    {
        if (wait_pin_state(1, 100) != 0) return -1; // On attend que ça passe en HIGH
        
        delay_tim(30); 
        
        if (GPIOA->IDR & (0x1 << DHT_PIN)) // Si le signal est repassé à 0 c'est qu'il durait moins de 30us donc c'était un 0.
        {                                  // Si le signal est toujours à 1 c'est qu'il durait + de 30us donc c'était un 1.
            raw_bytes[i / 8] |= (1 << (7 - (i % 8)));
        }
        if (wait_pin_state(0, 100) != 0) return -1;
    }
    
    // Le cinquième octet est un checksum.
    uint8_t checksum = raw_bytes[0] + raw_bytes[1] + raw_bytes[2] + raw_bytes[3];
    if (checksum != raw_bytes[4])
    {
        return -2; 
    }

    // L'humidité est représentée par les deux premiers octets.
    uint16_t humi_raw = (raw_bytes[0] << 8) | raw_bytes[1];
    data->humidity_int = humi_raw / 10;
    data->humidity_dec = humi_raw % 10;
    
    // La température est représentée par les deux octets d'après.
    uint16_t temp_raw = (raw_bytes[2] << 8) | raw_bytes[3];
    
    // Gestion du signe négatif (Avec un DHT22, le bit le plus haut à 1 signifie négatif)
    if (temp_raw & 0x8000) 
    {
        temp_raw &= 0x7FFF; // On retire le bit de signe pour garder la valeur
    }
    
    data->temperature_int = temp_raw / 10;
    data->temperature_dec = temp_raw % 10;
    
    return 0;
}