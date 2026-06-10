#include "stm32g431xx.h"
#include <stm32g4xx.h>
#include <string.h>
#include <stdio.h>

#define TX_PIN 2
#define RX_PIN 15
#define BAUDRATE 9600

void init_uart(void);
void send_c(char c);
char recv_c(void);
void send_string(char* s);
