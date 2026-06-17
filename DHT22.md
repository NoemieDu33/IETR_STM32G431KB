# STM32G431KB

[Documentation du STM32G431KB](https://www.farnell.com/datasheets/3182254.pdf)
[Datasheet du STM32G431KB](https://www.st.com/resource/en/datasheet/stm32g431c6.pdf) (notamment pour les Alternate Functions)

Rappel des opérateurs binaires
* ``&=`` applique un masque AND. Par exemple ``0xE &= 0x3`` donne ``0x2``.
* ``|=`` applique un masque OR. Par exemple ``0xE |= 0x3`` donne ``0xF``.
* ``^=`` applique un masque XOR. Par exemple ``0xE ^= 0x3`` donne ``0xD``.
* ``~`` inverse un nombre. Par exemple, ``0xE &= ~(0x3)`` donne ``0xB``.
* ``<<`` fait un shift de N bits vers la gauche. Par exemple, ``0x2000 |= (0x1 << 2 )`` renvoie ``0x2100``. 

# DHT22

Capteur de température et humidité à 1 broche de données
Module nécessaire : TIMER

## Fonctionnement du DHT22 et précisions

Le DHT22 doit être alimenté en 3.3V.
Chaque bit est représenté par un signal HIGH avec une durée précise. 28us = 0, 70us = 1

- Il faut d'abord passer le data wire en INPUT et envoyer un signal LOW pendant 18ms.
- Ensuite, on le passe en OUTPUT pour écouter son ACKNOWLEDGE : 0 -> 1 -> 0 
- Ensuite, on a 4 octets de données. Le cinquième octet est un CHECKSUM. 

## Setup de l'horloge (HSI)

* Il faut que les horloges des ``GPIOA`` et ``GPIOB`` soient activées (dans le registre ``RCC_AHB2ENR``).

* Dans le registre de contrôle d'horloge ``RCC_CR`` on peut activer l'horloge ``HSI`` (High Speed Internal, 16MHz) ainsi que lire le flag ``HSIRDY`` pour savoir si l'horloge ``HSI`` est correctement initialisée.
* On va ensuite setup l'horloge ``HSI`` en tant que ``SYSCLK`` (horloge du système) dans le registre ``CCIPR`` puis ``CFGR``.
* Enfin, dans ce même registre ``CFGR``, on peut modifier les fréquences vers les différents bus (AHB, APB1, APB2). On n'a aucun intérêt à mettre des divisions ici.

## Setup du TIMER

* Tout d'abord on va optimiser la CLK. Dans le registre ``FLASH_ACR``, on peut régler la latency (utile si on a une clock très rapide du genre 170MHz), et définir des caches (ICEN, DCEN) pour améliorer la vitesse de lecture.
* Enfin, on pense à désactiver l'horlose HSE dans le registre ``RCC_CR``.


* On active le timer qu'on souhaite dans le registre ``RCC_APB1ENR1``.
* On définit le prescaler dans le registre ``TIMx_PSC`` et l'automatic reload (le temps que prend un cycle avant de revenir à 0, vu qu'un timer compte en permanence) dans le registre ``TIMx_ARR``.
* On peut manuellement update les générations du timer avec le bit ``UG`` du registre ``TIMx_EGR``.
* Enfin, on démarre le timer dans le registre``TIMx_CR1``.
* On récupère la valeur de notre timer dans le registre ``TIMx_CNT``
