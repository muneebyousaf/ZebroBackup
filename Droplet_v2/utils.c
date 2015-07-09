/*
 * utils.c
 *
 *  Created on: Oct 23, 2012
 *      Author: samuezih
 */
#include <math.h>
#include "utils.h"
#include <string.h>

void mydelay(volatile uint32_t count) {
	while (count) {
		count--;
	}
}

void wait_us(uint32_t us) {
	mydelay(us * WAIT_OP_US);
}

void wait_ms(uint32_t ms) {
	wait_us(ms * 1000);
}

typedef union {
long	L;
float	F;
}	 LF_t;

char *ftoa(float f) //, int *status)
{
	long mantissa, int_part, frac_part;
	short exp2;
	LF_t x;
	char *p;
	static char outbuf[15];

	//*status = 0;
	if (f == 0.0)
	{
		outbuf[0] = '0';
		outbuf[1] = '.';
		outbuf[2] = '0';
		outbuf[3] = 0;
		return outbuf;
	}
	x.F = f;

	exp2 = (unsigned char)(x.L >> 23) - 127;
	mantissa = (x.L & 0xFFFFFF) | 0x800000;
	frac_part = 0;
	int_part = 0;

	if (exp2 >= 31)
	{
		//*status = _FTOA_TOO_LARGE;
		return 0;
	}
	else if (exp2 < -23)
	{
		//*status = _FTOA_TOO_SMALL;
		return 0;
	}
	else if (exp2 >= 23)
	int_part = mantissa << (exp2 - 23);
	else if (exp2 >= 0)
	{
		int_part = mantissa >> (23 - exp2);
		frac_part = (mantissa << (exp2 + 1)) & 0xFFFFFF;
	}
	else /* if (exp2 < 0) */
	frac_part = (mantissa & 0xFFFFFF) >> -(exp2 + 1);

	p = outbuf;

	if (x.L < 0)
		*p++ = '-';

	if (int_part == 0)
		*p++ = '0';
	else
	{
		ltoa(p, int_part, 10);
		while (*p)
		p++;
	}
	*p++ = '.';

	if (frac_part == 0)
		*p++ = '0';
	else
	{
		char m, max;

		max = sizeof (outbuf) - (p - outbuf) - 1;
		if (max > 7)
			max = 7;
		/* print BCD */
		for (m = 0; m < max; m++)
		{
			/* frac_part *= 10;	*/
			frac_part = (frac_part << 3) + (frac_part << 1);

			*p++ = (frac_part >> 24) + '0';
			frac_part &= 0xFFFFFF;
		}
		/* delete ending zeroes */
		for (--p; p[0] == '0' && p[-1] != '.'; --p)
			;
			++p;
	}
	*p = 0;

	return outbuf;
}

void ltoa(char *buf, unsigned long i, int base)
{
	char *s;
	#define LENGTHLTOA	25
	int rem;
	char rev[LENGTHLTOA+1];

	if (i == 0)
		s = "0";
	else
		{
		rev[LENGTHLTOA] = 0;
		s = &rev[LENGTHLTOA];
		while (i)
			{
			rem = i % base;
			if (rem < 10)
				*--s = rem + '0';
			else if (base == 16)
				*--s = "abcdef"[rem - 10];
			i /= base;
			}
		}
	strcpy(buf, s);
}

void itoa(char *buf, unsigned int i, int base)
{
	char *s;
	#define LENGTHITOA	10
	int rem;
	char rev[LENGTHITOA+1];

	if (i == 0)
		(*s) = '0';
	else
		{
		rev[LENGTHITOA] = 0;
		s = &rev[LENGTHITOA];
		while (i)
			{
				rem = i % base;
				if (rem < 10)
					*--s = rem + '0';
				else if (base == 16)
					*--s = "abcdef"[rem - 10];

				i /= base;
			}
		}
	strcpy(buf, s);
}

//int cin(unsigned timeout) {
//	uint8_t c;
//	int ret = -1;
//
//	timer[TIMER_CIN] = timeout;
//
//	do {
//		/* try to fetch a byte */
//		if (cdc_read(&c, 1) > 0) {
//			ret = c;
//			break;
//		}
//
//	} while (timer[TIMER_CIN] > 0);
//
//	return ret;
//}
//
//void cout(uint8_t *buf, unsigned len) {
//	unsigned sent;
//
//	uint32_t start_time = get_boot_time_ms();
//	while (len) {
//		sent = cdc_write(buf, len);
//		len -= sent;
//		buf += sent;
//
//		if (get_boot_time_ms() - start_time > 1) {
//			led_on(LED_BOOTLOADER);
////			cdc_disconnect();
////			cdc_reconnect();
////			cdc_reset();
//			return;
//		}
//
//	}
//}
