#ifndef __CLOCK__
#define __CLOCK__

#include <avr/interrupt.h>

#define CLK 8000000

typedef unsigned long long timestamp;

timestamp __clock__ = 0;

#define PRESCALER 256
#define TICKS (CLK / PRESCALER)

#define INTS_P_SEC (TICKS / 256)
#define RATE (1000 / INTS_P_SEC)

ISR(TIMER0_OVF_vect) {
    __clock__ += RATE;
}

void clock_init(void) {
	TCCR0 = 0b100; // 31 250 ticks/s
	TIMSK |= 1;
	sei();
}

timestamp clock(void) {
	return __clock__;
}
	
#endif