# STM32G431KB

[Documentation du STM32G431KB](https://www.farnell.com/datasheets/3182254.pdf)

Rappel des opérateurs binaires
* ``&=`` applique un masque AND. Par exemple ``0xE &= 0x3`` donne ``0x2``.
* ``|=`` applique un masque OR. Par exemple ``0xE |= 0x3`` donne ``0xF``.
* ``^=`` applique un masque XOR. Par exemple ``0xE ^= 0x3`` donne ``0xD``.
* ``~`` inverse un nombre. Par exemple, ``0xE &= ~(0x3)`` donne ``0xB``.
* ``<<`` fait un shift de N bits vers la gauche. Par exemple, ``0x2000 |= (0x1 << 2 )`` renvoie ``0x2100``. 

# UART

Bus utilisé: USART2

## Setup du bus UART

### Général

* Il faut que les horloges des ``GPIOA`` et ``GPIOB`` soient activées.
* On active l'horloge du USART2 dans le registre ``RCC_APB1ENR1``
* On définit la baudrate dnas le registre ``USART_BRR``
* On active l'USART dans le registre ``USART_CR1``

### Transmission (TX)

* On configure notre pin de transmission dans le registre ``GPIOx_MODER``. On le setup en Alternate.
* On sélectionne ensuite notre fonction alternate dans le registre ``GPIOx_AFRH`` (noté ``AFR[0]`` en C) pour les pins 0 à 7, et ``GPIOx_AFRL`` (noté ``AFR[1]`` en C) pour les pins 8 à 15.
* On active la transmission dans le registre ``USART_CR1``
* Les flags de statut sont dans le registre ``USART_ISR``. On peut observer le flag ``TEACK`` pour savoir s'il est bien initialisé, et le flag ``TXFNF`` pour savoir si on peut transmettre des données.
* Pour envoyer des données, il faut les écrire dans le registre ``USART_TDR``

### Réception (RX)

* On configure notre pin de réception dans le registre ``GPIOx_MODER``. On le setup en Alternate.
* On sélectionne ensuite notre fonction alternate dans le registre ``GPIOx_AFRH`` (noté ``AFR[0]`` en C) pour les pins 0 à 7, et ``GPIOx_AFRL`` (noté ``AFR[1]`` en C) pour les pins 8 à 15.
* On active la réception dans le registre ``USART_CR1``
* Les flags de statut sont dans le registre ``USART_ISR``. On peut observer le flag ``REACK`` pour savoir s'il est bien initialisé, et le flag ``RXFNE`` pour savoir si des données ont été reçues.
* Les données reçues seront dans le registre ``USART_RDR``