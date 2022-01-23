#include "wiegand.h"

uint32_t getCode(Wiegand *w)
{
	return w->code;
}

int16_t getWiegandType(Wiegand *w)
{
	return w->wiegandType;
}

void ReadD0(Wiegand *w)
{
	w->bitCount++;				// Increament bit count for Interrupt connected to D0

	if (w->bitCount > 31)			// If bit count more than 31, process high bits
	{
		w->cardTempHigh |= ((0x80000000 & w->cardTemp) >> 31);	//	shift value to high bits
		w->cardTempHigh <<= 1;
		w->cardTemp <<= 1;
	}
	else
	{
		w->cardTemp <<= 1;		// D0 represent binary 0, so just left shift card data
	}

	w->lastWiegand = HAL_GetTick();	// Keep track of last wiegand bit received
}

void ReadD1(Wiegand *w)
{
	w->bitCount++;				// Increment bit count for Interrupt connected to D1

	if(w->bitCount > 31)			// If bit count more than 31, process high bits
	{
		w->cardTempHigh |= ((0x80000000 & w->cardTemp) >> 31);	// shift value to high bits
		w->cardTempHigh <<= 1;
		w->cardTemp |= 1;
		w->cardTemp <<= 1;
	}
	else
	{
		w->cardTemp |= 1;			// D1 represent binary 1, so OR card data with 1 then
		w->cardTemp <<= 1;		// left shift card data
	}

	w->lastWiegand = HAL_GetTick();	// Keep track of last wiegand bit received
}

uint32_t GetCardId(volatile uint32_t *codehigh, volatile uint32_t *codelow, uint8_t bitlength)
{

	if(bitlength == 26)								// EM tag
	{
		return (*codelow & 0x1FFFFFE) >> 1;
	}

	if(bitlength == 34)								// Mifare
	{
		*codehigh = *codehigh & 0x03;				// only need the 2 LSB of the codehigh
		*codehigh <<= 30;							// shift 2 LSB to MSB
		*codelow >>= 1;
		return *codehigh | *codelow;
	}

	return *codelow;								// EM tag or Mifare without parity bits
}

uint8_t translateEnterEscapeKeyPress(uint8_t originalKeyPress)
{
	switch(originalKeyPress)
	{
		case 0x0b:        // 11 or * key
			return 0x0d;  // 13 or ASCII ENTER

		case 0x0a:        // 10 or # key
			return 0x1b;  // 27 or ASCII ESCAPE

		default:
			return originalKeyPress;
	}
}

uint8_t wig_available(Wiegand *w)
{
	uint32_t cardID;
	uint32_t time_wig = HAL_GetTick();

	if((time_wig - w->lastWiegand) > 25) // if no more signal coming through after 25ms
	{
		if((w->bitCount == 24) || (w->bitCount == 26) || (w->bitCount == 32) || (w->bitCount == 34) || (w->bitCount == 8) || (w->bitCount == 4)) // bitCount for keypress=4 or 8, Wiegand 26=24 or 26, Wiegand 34=32 or 34
		{
			w->cardTemp >>= 1; // shift right 1 bit to get back the real value - interrupt done 1 left shift in advance

			if(w->bitCount > 32) // bit count more than 32 bits, shift high bits right to make adjustment
			{
				w->cardTempHigh >>= 1;
			}

			if(w->bitCount == 8)		// keypress wiegand with integrity
			{
				// 8-bit Wiegand keyboard data, high nibble is the "NOT" of low nibble
				// eg if key 1 pressed, data=E1 in binary 11100001 , high nibble=1110 , low nibble = 0001
				uint8_t highNibble = (w->cardTemp & 0xf0) >> 4;
				uint8_t lowNibble = (w->cardTemp & 0x0f);
				w->wiegandType = w->bitCount;
				w->bitCount = 0;
				w->cardTemp = 0;
				w->cardTempHigh = 0;

				if(lowNibble == (~highNibble & 0x0f))		// check if low nibble matches the "NOT" of high nibble.
				{
					w->code = (int16_t)translateEnterEscapeKeyPress(lowNibble);
					return 1;
				}
				else
				{
					w->lastWiegand = time_wig;
					w->bitCount = 0;
					w->cardTemp = 0;
					w->cardTempHigh = 0;
					return 0;
				}

				// TODO: Handle validation failure case!
			}
			else if(4 == w->bitCount)
			{
				// 4-bit Wiegand codes have no data integrity check so we just
				// read the LOW nibble.
				w->code = (int16_t)translateEnterEscapeKeyPress(w->cardTemp & 0x0000000F);

				w->wiegandType = w->bitCount;
				w->bitCount = 0;
				w->cardTemp = 0;
				w->cardTempHigh = 0;

				return 1;
			}
			else		// wiegand 26 or wiegand 34
			{
				cardID = GetCardId(&w->cardTempHigh, &w->cardTemp, w->bitCount);
				w->wiegandType = w->bitCount;
				w->bitCount = 0;
				w->cardTemp = 0;
				w->cardTempHigh = 0;
				w->code = cardID;
				return 1;
			}
		}
		else
		{
			// well time over 25 ms and bitCount !=8 , !=26, !=34 , must be noise or nothing then.
			w->lastWiegand = time_wig;
			w->bitCount = 0;
			w->cardTemp = 0;
			w->cardTempHigh = 0;
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
