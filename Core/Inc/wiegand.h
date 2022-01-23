#ifndef WIEGAND_H_
#define WIEGAND_H_

#include "main.h"

typedef struct {
    volatile uint32_t cardTempHigh;
    volatile uint32_t cardTemp;
    volatile uint32_t lastWiegand;
    uint32_t code;
    volatile int16_t bitCount;
    int16_t wiegandType;
} Wiegand;

uint32_t getCode(Wiegand *w);
int16_t getWiegandType(Wiegand *w);
void ReadD0(Wiegand *w);
void ReadD1(Wiegand *w);
uint32_t GetCardId(volatile uint32_t *codehigh, volatile uint32_t *codelow, uint8_t bitlength);
uint8_t wig_available(Wiegand *w);

#endif /* WIEGAND_H_ */
