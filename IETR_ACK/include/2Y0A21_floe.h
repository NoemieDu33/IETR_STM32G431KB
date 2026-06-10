#include "stm32g4xx.h"

#define TELEMETRE_PIN 0 // GPIOA

void init_2Y0A21(void);

uint16_t read_2Y0A21(void);

float adc_to_distance_cm(uint16_t adc_val);

