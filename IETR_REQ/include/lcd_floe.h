#include "stm32g431xx.h"
#include <stm32g4xx.h>

// Tout est sur le GPIO B sauf E qui est sur le GPIO A.
#define RS 0 
#define RW 7
#define D7 6
#define D6 5
#define D5 4
#define D4 3
#define E 7

#define DELAY 10000 // Le LCD a des difficutés si c'est + rapide que ça


void init_pins_lcd(void);

void reg_command();

void send_command_4b(uint8_t cmd);
void send_command_8b(uint8_t cmd);

void hard_reset();

void init_16x2(void);

void newline();
void scroll_left();

void send_1(GPIO_TypeDef* gpio, int pin);
void send_0(GPIO_TypeDef* gpio, int pin);
void data_1(void);
void data_0(void);

void send_letter(char c);
void send_text(char* s);

void clear_screen(void);
