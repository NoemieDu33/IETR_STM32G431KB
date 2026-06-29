#include "ssd1306_floe.h"
#include "stm32g431xx.h"
#include <string.h>
#include "delay_floe.h"
#include <stdio.h>


#define NEWLINE_TAG '-' // Caractère qui cause un saut de ligne. Par défaut '-'
#define END_RX_TAG '\n' // Caractère qui coupe la réception d'un msg. Par défaut '\n'


// Macros explicites pour la maintenance des broches I2C2

// ============================================================================
// CONFIGURATION & VARIABLES GLOBALES
// ============================================================================
volatile int graphtab[61] = {0};

// Séquence d'initialisation standard pour SSD1306 (128x64)
const uint8_t SSD1306_INIT_SEQ[] = {
    0xAE,       // 1. Éteindre l'écran (Display OFF)
    0xD5, 0x80, // 2. Définir la fréquence d'horloge de l'affichage
    0xA8, 0x3F, // 3. Définir le Multiplex Ratio (64 lignes pour 128x64)
    0xD3, 0x00, // 4. Définir le Display Offset (Pas de décalage)
    0x40,       // 5. Définir la ligne de départ de la RAM à 0
    0x8D, 0x14, // 6. CONFIGURATION CRITIQUE : Activer la pompe de charge interne
    0x20, 0x00, // 7. Définir le mode d'adressage mémoire en Horizontal
    0xA1,       // 8. Remappage des segments (Inverse l'axe X pour bonne orientation)
    0xC8,       // 9. Sens du scan des lignes (Inverse l'axe Y)
    0xDA, 0x12, // 10. Configurer les broches matérielles COM (Alterné)
    0x81, 0xCF, // 11. Régler le contraste / luminosité
    0xD9, 0xF1, // 12. Définir la période de pré-charge
    0xDB, 0x40, // 13. Régler le niveau de désélection VCOMH
    0xA4,       // 14. Reprendre l'affichage depuis la RAM
    0xA6,       // 15. Mode d'affichage normal (1 = pixel allumé)
    0xAF        // 16. Allumer l'écran !
};

// Buffer local représentant les pixels de l'écran (128x64 bits = 1024 octets)
static uint8_t OLED_Buffer[1024];

// Table de caractères ASCII complète (Du caractère 0x20 'Spc' au 0x7E '~')
const uint8_t Font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, 
    {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7F, 0x14, 0x7F, 0x14}, 
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62}, 
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, 
    {0x00, 0x1C, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1C, 0x00}, 
    {0x14, 0x08, 0x3E, 0x08, 0x14}, {0x08, 0x08, 0x3E, 0x08, 0x08}, 
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, 
    {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02}, 
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00}, 
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, 
    {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39}, 
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03}, 
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, 
    {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00}, 
    {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14}, 
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06}, 
    {0x32, 0x49, 0x79, 0x41, 0x3E}, {0x7E, 0x11, 0x11, 0x11, 0x7E}, 
    {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22}, 
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, 
    {0x7F, 0x09, 0x09, 0x09, 0x01}, {0x3E, 0x41, 0x49, 0x49, 0x7A}, 
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00}, 
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, 
    {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x0C, 0x02, 0x7F}, 
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E}, 
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, 
    {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31}, 
    {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F}, 
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, 
    {0x63, 0x14, 0x08, 0x14, 0x63}, {0x07, 0x08, 0x70, 0x08, 0x07}, 
    {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x00}, 
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7F, 0x00}, 
    {0x04, 0x02, 0x01, 0x02, 0x04}, {0x40, 0x40, 0x40, 0x40, 0x40}, 
    {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78}, 
    {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, 
    {0x38, 0x44, 0x44, 0x48, 0x7F}, {0x38, 0x54, 0x54, 0x54, 0x18}, 
    {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x0C, 0x52, 0x52, 0x52, 0x3E}, 
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, 
    {0x20, 0x40, 0x44, 0x3D, 0x00}, {0x7F, 0x10, 0x28, 0x44, 0x00}, 
    {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78}, 
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, 
    {0x7C, 0x14, 0x14, 0x14, 0x08}, {0x08, 0x14, 0x14, 0x18, 0x7C}, 
    {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20}, 
    {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, 
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, {0x3C, 0x40, 0x30, 0x40, 0x3C}, 
    {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C}, 
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, 
    {0x00, 0x00, 0x7F, 0x00, 0x00}, {0x00, 0x41, 0x36, 0x08, 0x00}, 
    {0x10, 0x08, 0x18, 0x10, 0x08}  
};

void init_pins_oled(void){
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN; 
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;  

    GPIOA->MODER &= ~((3 << (OLED_SDA_PIN * 2)) | (3 << (OLED_SCL_PIN * 2))); 
    GPIOA->MODER |=  ((2 << (OLED_SDA_PIN * 2)) | (2 << (OLED_SCL_PIN * 2)));

    GPIOA->OTYPER |= (1 << OLED_SDA_PIN) | (1 << OLED_SCL_PIN);

    GPIOA->OSPEEDR |= (3 << (OLED_SDA_PIN * 2)) | (3 << (OLED_SCL_PIN * 2));

    GPIOA->PUPDR &= ~((3 << (OLED_SDA_PIN * 2)) | (3 << (OLED_SCL_PIN * 2)));
    GPIOA->PUPDR |=  ((1 << (OLED_SDA_PIN * 2)) | (1 << (OLED_SCL_PIN * 2)));

    GPIOA->AFR[1] &= ~( (0xFUL << ((OLED_SCL_PIN - 8) * 4)) | (0xFUL << ((OLED_SDA_PIN - 8) * 4)) ); 
    GPIOA->AFR[1] |=  ( (4UL   << ((OLED_SCL_PIN - 8) * 4)) | (4UL   << ((OLED_SDA_PIN - 8) * 4)) );

    I2C2->CR1 &= ~I2C_CR1_PE;
    I2C2->TIMINGR = 0xF0420F13; //0x00300F33
    I2C2->CR1 |= I2C_CR1_PE;
}
void I2C2_BurstWrite(uint8_t device_addr, uint8_t control_byte, uint8_t *payload, uint8_t length) {
    while (I2C2->ISR & I2C_ISR_BUSY);

    // length + 1 (128 + 1 = 129), ce qui rentre parfaitement dans les 8 bits de NBYTES
    I2C2->CR2 = (device_addr & 0xFE) | 
                ((length + 1) << I2C_CR2_NBYTES_Pos); 

    I2C2->CR2 |= I2C_CR2_START;

    // Envoi du control byte
    while (!(I2C2->ISR & I2C_ISR_TXIS)) {
        if (I2C2->ISR & I2C_ISR_NACKF) { I2C2->ICR |= I2C_ICR_NACKCF; I2C2->CR2 |= I2C_CR2_STOP; return; }
    } 
    I2C2->TXDR = control_byte;

    // Envoi du paquet
    for (uint8_t i = 0; i < length; i++) {
        while (!(I2C2->ISR & I2C_ISR_TXIS)) {
            if (I2C2->ISR & I2C_ISR_NACKF) { I2C2->ICR |= I2C_ICR_NACKCF; I2C2->CR2 |= I2C_CR2_STOP; return; }
        }
        I2C2->TXDR = payload[i];
    }

    // Attente de la fin du paquet et génération du STOP
    while (!(I2C2->ISR & I2C_ISR_TC));
    I2C2->CR2 |= I2C_CR2_STOP;

    while (!(I2C2->ISR & I2C_ISR_STOPF));
    I2C2->ICR |= I2C_ICR_STOPCF;
}

void init_oled(void){
    init_pins_oled();
    delay(DELAY); 
    
    I2C2_BurstWrite(0x78, 0x00, (uint8_t*)SSD1306_INIT_SEQ, sizeof(SSD1306_INIT_SEQ));
    
    clear_screen_o();
    update_screen();
}

void clear_screen_o(void){
    // Remplit le tableau local avec des zéros (pixels éteints)
    memset(OLED_Buffer, 0x00, sizeof(OLED_Buffer));
}

void update_screen(void){
    // Réinitialise les coordonnées à (0,0)
    uint8_t reset_boundaries[] = {
        0x21, 0x00, 127, 
        0x22, 0x00, 7    
    };
    I2C2_BurstWrite(0x78, 0x00, reset_boundaries, sizeof(reset_boundaries));

    // Envoi du buffer par blocs de 128 octets (8 * 128 = 1024)
    for(uint16_t i = 0; i < 1024; i += 128) {
         I2C2_BurstWrite(0x78, 0x40, &OLED_Buffer[i], 128);
    }
}

void send_px(int posx, int posy){
    // Sortie des limites physiques de l'écran (128x64) -> Sécurité
    if (posx < 0 || posx >= 128 || posy < 0 || posy >= 64) {
        return; 
    }

    // Calcul de l'index dans le repère SSD1306 Horizontal
    OLED_Buffer[posx + (posy / 8) * 128] |= (1 << (posy % 8));
}

void send_letter_o(char c, int posx, int posy){
    if (c < 0x20 || c > 0x7E) return; 

    const uint8_t *glyph = Font5x7[c - 0x20];

    for (int i = 0; i < 5; i++) {       // Largeur de 5 pixels
        for (int j = 0; j < 8; j++) {   // Hauteur maximale de 8 pixels
            if (glyph[i] & (1 << j)) {
                send_px(posx + i, posy + j);
            }
        }
    }
}

void send_text_o(char* s, int posx, int posy){
    int initial_x = posx;
    while (*s) {
        if (*s == END_RX_TAG){
            return;
        } else if (*s == NEWLINE_TAG){
            posx = initial_x;
            posy = posy+9;
        } else {
            send_letter_o(*s, posx, posy);
            posx += 6; // Largeur du caractère (5px) + 1px d'espacement
        }
        s++;
    }
}

void send_num_o(int val, int posx, int posy) {
    char str_num[12]; // Assez grand pour stocker un int 32-bit
    snprintf(str_num, sizeof(str_num), "%d", val);
    send_text_o(str_num, posx, posy);
}

void draw_welcome_o(void){
    send_text_o("Bouton 1:TEST--Bouton 2:Temperature--Bouton 3:Analyse-         2Y0A21\n", 5, 10);
}

void draw_template_o(void){
    send_text_o("4095", 1, 1);
    send_text_o("0", 8, 56);
    send_text_o("ADC", 95, 1);
    send_text_o("Viewer", 95, 10);
    for (int i = 0; i < 64; i++){
        if (i > 12 && i < 48){
            send_px(10, i);
        }
        send_px(i + 27, 0);
        send_px(i + 27, 63);
        send_px(27, i);
        send_px(90, i);
    }
    send_px(9, 14);  send_px(11, 14);
    send_px(9, 15);  send_px(11, 15);
    send_px(8, 15);  send_px(12, 15);
}

void clear_zone_o(int start_x, int start_y, int width, int height) {
    for (int x = start_x; x < start_x + width; x++) {
        for (int y = start_y; y < start_y + height; y++) {
            if (x >= 0 && x < 128 && y >= 0 && y < 64) {
                OLED_Buffer[x + (y / 8) * 128] &= ~(1 << (y % 8));
            }
        }
    }
}

void graph_o(int val){
    for (int i = 0; i < 60; i++){
        graphtab[i] = graphtab[i+1];
    }
    graphtab[60] = val;

    clear_zone_o(28, 1, 61, 61);
    clear_zone_o(98, 30, 20, 20);

    for (int i = 0; i < 61; i++){
        send_px(28 + i, 62 - graphtab[i]);
    }

    send_num_o(val, 100, 32);
}