# STM32G431KB

[Documentation du STM32G431KB](https://www.farnell.com/datasheets/3182254.pdf)
[Datasheet du STM32G431KB](https://www.st.com/resource/en/datasheet/stm32g431c6.pdf) (notamment pour les Alternate Functions)

Rappel des opérateurs binaires
* ``&=`` applique un masque AND. Par exemple ``0xE &= 0x3`` donne ``0x2``.
* ``|=`` applique un masque OR. Par exemple ``0xE |= 0x3`` donne ``0xF``.
* ``^=`` applique un masque XOR. Par exemple ``0xE ^= 0x3`` donne ``0xD``.
* ``~`` inverse un nombre. Par exemple, ``0xE &= ~(0x3)`` donne ``0xB``.
* ``<<`` fait un shift de N bits vers la gauche. Par exemple, ``0x2000 |= (0x1 << 2 )`` renvoie ``0x2100``. 

# UART

Bus UART utilisé: USART2

## Setup du bus UART

### Général

* Il faut que les horloges des ``GPIOA`` et ``GPIOB`` soient activées (dans le registre ``RCC_AHB2ENR``).
* On active l'horloge du USART2 dans le registre ``RCC_APB1ENR1``
* On définit la baudrate dans le registre ``USART_BRR``
* On active l'USART dans le registre ``USART_CR1``

### Transmission (TX)

* On configure notre broche de transmission dans le registre ``GPIOx_MODER``. On le setup en Alternate Function.
* On sélectionne ensuite notre fonction alternate dans le registre ``GPIOx_AFRH`` (noté ``AFR[0]`` en C) pour les broches 0 à 7, et ``GPIOx_AFRL`` (noté ``AFR[1]`` en C) pour les broches 8 à 15.
* On active la transmission dans le registre ``USART_CR1``
* Les flags de statut sont dans le registre ``USART_ISR``. On peut observer le flag ``TEACK`` pour savoir s'il est bien initialisé. Le flag ``TXFNF`` permet de savoir si on peut transmettre des données (Transmit Data Register Empty).
* Pour envoyer des données, il faut les écrire dans le registre ``USART_TDR``

### Réception (RX)

* On configure notre broche de réception dans le registre ``GPIOx_MODER``. On le setup en Alternate Function.
* On sélectionne ensuite notre fonction alternate dans le registre ``GPIOx_AFRH`` (noté ``AFR[0]`` en C) pour les broches 0 à 7, et ``GPIOx_AFRL`` (noté ``AFR[1]`` en C) pour les broches 8 à 15. 
* On active la réception dans le registre ``USART_CR1``
* Les flags de statut sont dans le registre ``USART_ISR``. On peut observer le flag ``REACK`` pour savoir s'il est bien initialisé. Le flag ``RXFNE`` permet de  savoir si des données ont été reçues (Recieved Data Register Not Empty).
* Les données reçues seront dans le registre ``USART_RDR``. Consulter ce registre va reset le flag ``RXFNE``.