#ifndef __DT__
#define __DT__

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>

#define DT_PORT PORTD
#define DT_DDR DDRD
#define DT_PIN PIND
#define DT_SENSOR 1
#define NOID 0xCC
#define T_CONVERT 0x44
#define READ_DATA 0xBE
#define CONV_TIME 750
#define WRITE_SCRATCHPAD 0x4E

char dt_test(void) {
	unsigned char stack = SREG;
	cli();
	char dt;
	DT_DDR |= 1 << DT_SENSOR;
	_delay_us(500);
	DT_DDR &= ~(1 << DT_SENSOR);
	_delay_us(70);
	if ((DT_PIN & (1 << DT_SENSOR)) == 0) {
		dt = 1;
	} else {
		dt = 0;
	}
	SREG = stack;
	_delay_us(420);
	return dt;
}

char dt_rx(void) {
	unsigned char stack = SREG;
	cli();
	char bit;
	DT_DDR |= 1 << DT_SENSOR;
	_delay_us(2);
	DT_DDR &= ~(1 << DT_SENSOR);
	_delay_us(14);
	bit = (DT_PIN & (1 << DT_SENSOR)) >> DT_SENSOR;
	_delay_us(45);
	SREG = stack;
	return bit;
}

unsigned char dt_rx8(void) {
	char c = 0;
	for (char i = 0; i < 8; i++) c |= dt_rx() << i;
	return c;
}

void dt_tx(char b) {
	char stack = SREG;
	cli();
	DT_DDR |= 1 << DT_SENSOR;
	_delay_us(2);
	if (b) DT_DDR &= ~(1 << DT_SENSOR);
	_delay_us(65);
	DT_DDR &= ~(1 << DT_SENSOR);
	SREG = stack;
}

void dt_tx8(unsigned char b) {
	for (char i = 0; i < 8; i++) {
		if ((b &(1 << i)) == (1 << i)) {
			dt_tx(1);
		} else {
			dt_tx(0);
		}
	}
}

void dt_convert(void) {
	if (!dt_test()) return;
	dt_tx8(NOID);
	dt_tx8(T_CONVERT);
	// then conversion delay
}

int dt_read(void) {
	unsigned char l;
	unsigned int h = 0;
	if (!dt_test()) return 0;
	dt_tx8(NOID);
	dt_tx8(READ_DATA);
	l = dt_rx8();
	h = dt_rx8();
	h = (h << 8) | l;
	return h;
}

unsigned char t_integer_part(int t) {
	unsigned char l = t & 0xFFFF;
	unsigned char h = (t >> 8) & 0xFFFF;
	
	return (l >> 4) | ((h & 0b111) << 4);
}

char t_sign(int t) {
	return (t >> 15) & 1;
}

# endif