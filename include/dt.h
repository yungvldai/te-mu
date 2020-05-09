#ifndef __DT__
#define __DT__

#include <avr/io.h>
#include <util/delay.h>

#define DT_PORT PORTD
#define DT_DDR  DDRD
#define DT_PIN  PIND
#define DT_SENSOR 1

#define NOID 0xCC
#define T_CONVERT 0x44
#define READ_DATA 0xBE

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

char dt_readbit(void) {
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

unsigned char dt_readbyte(void) {
  char c = 0;
  for (char i = 0; i < 8; i++ ) c |= dt_readbit() << i;
  return c;

}

void dt_sendbit(char bit) {
  char stack = SREG;
  cli();
	DT_DDR |= 1 << DT_SENSOR;
	_delay_us(2);
	if (bit) DT_DDR &= ~(1 << DT_SENSOR);
	_delay_us(65);
  DT_DDR &= ~(1 << DT_SENSOR);
  SREG = stack;
}

void dt_sendbyte(unsigned char bt) {
  for (char i = 0; i < 8; i++) {
    if ((bt & (1 << i)) == (1 << i)) {
			dt_sendbit(1);
    } else {
			dt_sendbit(0);
		}
  }
}

int dt_get(void) {
	unsigned char l;
	unsigned int h = 0;
  if (dt_test()) {
		dt_sendbyte(NOID);
		dt_sendbyte(T_CONVERT);
		_delay_ms(100);
		dt_test();
		dt_sendbyte(NOID);
		dt_sendbyte(READ_DATA);
		l = dt_readbyte();
		h = dt_readbyte();
		h = (h << 8) | l;
  }
  return h;
}

#endif