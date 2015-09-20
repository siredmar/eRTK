#ifndef OS_CFG_H_
#define OS_CFG_H_

#define TARGET_CPU ATMEGA2560

#define OS_MAX_NUMBER_OF_TASKS    (6u)  //anzahl definierter prozesse

#define OS_STACKSIZE     (256u)         //stack in byte bei 8 bit bzw. word bei 32 bit maschinen
#define OS_STACKLOWMARK  (240u)         //wenn debug aktiviert wird auf unterschreitung dieser marke geprueft


#endif
