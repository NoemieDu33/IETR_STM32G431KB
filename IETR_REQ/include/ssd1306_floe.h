#include "stm32g431xx.h"
#include <string.h>
#include "delay_floe.h"

#define SDA_PIN 7
#define SCL_PIN 15
#define DELAY 200   
    
void init_pins_oled(void);

void send_1(GPIO_TypeDef* gpio, int pin);

void send_0(GPIO_TypeDef* gpio, int pin);

void reg_command(void);

void init_oled(void);

void send_px(int posx, int posy);

void send_letter_o(char c, int posx, int posy);

void send_text_o(char* s, int posx, int posy);

void send_num_o(int val, int posx, int posy);

void clear_screen_o(void);

void update_screen(void);

void draw_template_o(void);

void draw_welcome_o(void);

void graph_o(int val);

void clear_zone_o(int start_x, int start_y, int width, int height);