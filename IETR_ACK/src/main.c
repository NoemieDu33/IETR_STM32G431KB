#include "stm32g431xx.h"
#include <stm32g4xx.h>
#include <string.h>
#include <stdio.h>

#include "2Y0A21_floe.h"
#include "DHT22_floe.h"
#include "led_floe.h"
#include "uart_floe.h"
#include "delay_floe.h"


// init_gpios active les horloges des GPIOA et GPIOB pour les différents modules utilisés. Doit être lancé en premier.
void init_gpios(void){
    RCC -> AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC -> AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
}

int main(void){
    init_gpios();
    setup_HSI_CLK(); // L'ordre d'iniailisation est important: HSI_CLK -> TIMER -> le reste
    init_TIMER();
    init_uart();
    init_led(); // La LED s'allume quand elle reçoit une requête et s'éteint quand une réponse est transmise. voir LED_ON et LED_OFF
    init_2Y0A21();
    init_DHT22();
    
    DHT22_Data_t dht_sensor_data;
    
    int status = 0;
    
    char buf[64];
    char ackbuf[16];
    int i=0;

    while(1)
    {
        char e = '\0';
        e = recv_c();
        if (!(e=='\0')){
            LED_ON();
            if (!(e=='\n')) {
                buf[i] = e;
                i++;
            } else {
                char acknum = buf[4];
                delay(1000000);


                if (acknum=='1'){ // Requête 1 : bouton de gauche = test
                    delay(1000000);
                    send_string("Ceci est un test-Je suis Floe!\n");
                    delay(500000);
                } else if (acknum=='2'){ // Requête 2 : bouton du milieu = DHT22 ici
                    
                    uint16_t buf1[16];
                    uint16_t buf2[16];
                    status = get_value_DHT22(&dht_sensor_data);
                    
                    if (status == 0)
                    {
                        LED_ON();
                        sprintf((char*)buf1,"Temp.: %u.%u C-",
                                        dht_sensor_data.temperature_int,
                                        dht_sensor_data.temperature_dec);
                        sprintf((char*)buf2,"Humid.: %u.%u%%\n",
                                        dht_sensor_data.humidity_int,
                                        dht_sensor_data.humidity_dec);

                        send_string((char*)buf1);
                        delay(500000);
                        send_string((char*)buf2);
                    }
                    else if (status == -1){
                        send_string("Timeout ERR\n");
                    }
                    else if (status == -2){
                        send_string("Checksum ERR\n");
                    }
                    

                    delay(2000000);
                
                } else if (acknum=='3'){ // Requête 3 : bouton de droite = télémètre ici

                    uint16_t buf1[16];
                    uint16_t buf2[16];

                    uint16_t val = read_2Y0A21();

                    float res = adc_to_distance_cm(val);
                    int entier = (int)res;
                    int dec = (int)((res - (float)entier) *10);

                    sprintf((char*)buf1,"DIST: %d.%d cm-", entier, dec);
                    sprintf((char*)buf2, "Valeur: %u\n", val);
                    send_string((char*)buf1);

                    delay(500000);
                    send_string((char*)buf2);
                    
                    delay(1000000);
                }


                i = 0;
                LED_OFF();
            }
        }
    }
}


    
        