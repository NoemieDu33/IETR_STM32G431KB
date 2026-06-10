#include "stm32g431xx.h"
#include "2Y0A21_floe.h"
#include "delay_floe.h"



// init_2Y0A21 initialise le CAN (ADC) qui permettra de lire les valeurs du télémètre. Il doit être appelé en premier.
void init_2Y0A21(void) {
    // On utilise l'horloge HSI comme horloge principale du MCU (SYSCLK) à 16HMz pour utiliser un ADC.
    // C'est important pour nos mesures sur le télémètre.
    if ((RCC->CR & RCC_CR_HSIRDY) == 0){ // HSIRDY = Horloge HSI activée (donc à 0 si elle n'est pas setup)
        RCC -> CR |= RCC_CR_HSION; // HSION = On active l'horloge HSI
        while((RCC->CR & RCC_CR_HSIRDY) == 0); // Puis on laisse l'horloge s'initialiser.
    }

    // Sélection du HSI pour l'ADC12
    // CCIPR = independant clock configuration register
    // Dans RCC_CCIPR, ADC 12 se situe aux bits 28 (0x1C) et 29 (0x1D)
   
    /*
    CCIPR est sur deux bits pour chaque valeur:
    00 = pas de clock
    01 = PLL 
    10 = SYSCLK
    11 = Reserved
    */

    RCC->CCIPR &= ~(RCC_CCIPR_ADC12SEL);
    RCC->CCIPR |=  (0x2 << 0x1c); // On setup SYSCLK sur CCIPR (Notre SYSCLK est HSI)

    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN; // ADC12 Enable
    delay(10);

    GPIOA->MODER &= ~(0x3 << 2*TELEMETRE_PIN); 
    GPIOA->MODER |= (0x3 << 2*TELEMETRE_PIN); // Mode Analogique
    GPIOA->PUPDR &= ~(0x3 << 2*TELEMETRE_PIN); // Pas de résistance de pull-up/down

    
    ADC12_COMMON->CCR &= ~(ADC_CCR_CKMODE); // CKMODE = 0 -> CLK asynchrone (configuée via CCIPR)
    ADC12_COMMON->CCR &= ~(ADC_CCR_PRESC); // Prescaler de l'ADC à 1 (= Pas de division)
    
  
    // ADC12_COMMON->CCR &= ~(ADC_CCR_VREFEN);

    ADC1->CR &= ~(ADC_CR_DEEPPWD); // Sortie du Deep Power Down
    ADC1->CR |= ADC_CR_ADVREGEN; // Allumage régulateur
    delay(50000);  

    
    ADC1->CR &= ~(ADC_CR_ADCALDIF); // ADCALDIF = mode de mesure différentiel (entre deux broches). 0 = Mesure par rapport au GND
    
    ADC1->CR |= ADC_CR_ADCAL;
    while(ADC1->CR & ADC_CR_ADCAL); // Calibration interne (mesure et correction des erreurs internes)

    ADC1->CFGR |= ADC_CFGR_CONT; // Conversion en mode continu 
 
    /*
    SQR1 est un registre sur 4 bits.

    Son premier mot de 4 bits est le registre L (en position 0x0):
    0000 = 1 conversion par séquence de conversions
    0001 = 2 conversions
    ...
    1111 = 16 conversions

    SQ1 est en position 0x6. 
    */

    ADC1->SQR1 &= (0x0 << 0x0); // 1 conversion (L=0x0)

    ADC1->SQR1 &= ~(ADC_SQR1_SQ1); 
    ADC1->SQR1 |= (0x1 << 0x6); // SQ1 a la valeur 0x1 = canal 1 de l'ADC (PA0)

    /*
    SMPR est sur 3 bits.
    C'est la sélection du temps d'échantillonnage (horloge de l'ADC comme référence)
    0x0 = 2.5 cycles 
    0x1 = 6.5 cycles
    0x2 = 12.5
    0x3 = 24.5
    0x4 = 47.5
    0x5 = 92.5
    0x6 = 247.5
    0x7 = 640.5
    */
    ADC1->SMPR1 &= ~(ADC_SMPR1_SMP1); // On setup l'échantillonnage
    ADC1->SMPR1 |=  (0x7 << 0x3); // SMP1 est du bit 0x3 au bit 0x5


    ADC1->ISR |= ADC_ISR_ADRDY; // On clear le flag Ready
    ADC1->CR |= ADC_CR_ADEN; // Activation de l'ADC
    while(!(ADC1->ISR & ADC_ISR_ADRDY)); // Attente de la stabilisation de l'ADC

    ADC1->CR |= ADC_CR_ADSTART; // Démarrage de la conversion continue
}

// read_2Y0A21 permet de récupérer la valeur de conversion du CAN.
// renvoie: un unsigned short
uint16_t read_2Y0A21(void) {

    //  Si un overrun a eu lieu (l'ADC a converti trop vite par rapport au CPU),
    // le registre DR se fige. On doit effacer ce flag pour débloquer l'ADC

    if (ADC1->ISR & ADC_ISR_OVR) {
        ADC1->ISR |= ADC_ISR_OVR; // Écrire 1 efface le flag OVR
    }

    while(!(ADC1->ISR & ADC_ISR_EOS)); // Attendre la fin de la séquence complète (EOS)
    
    ADC1->ISR |= ADC_ISR_EOS; // Ecrire 1 efface le flag EOS
    
    return (uint16_t)ADC1->DR; // Data Register, donc la valeur récupérée
}

float adc_to_distance_cm(uint16_t adc_val) {
 
    if (adc_val < 300) {
        return 80.0;
    }
    if (adc_val > 2400) { // TODO : affiner cette fonction 
        return 10.0;
    }

	return (24280.0f / ((float)adc_val + 28.0f));
}