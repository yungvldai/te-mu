
#ifndef __IO__
#define __IO__

#include <avr/io.h>

#define I_DDR DDRC
#define O_DDR DDRD
#define IO_PORT PORTD
#define IO_PIN PINC

typedef enum { A = 0, B = 1, C = 2, D = 3 } Button;
typedef enum { Z = 4, X = 5, Y = 6, W = 7 } Port;

void io_init(void) { 
	I_DDR = 0;
	O_DDR = 0b11110000;
}

void io_write(Port o, char value) {
	if (value) {
		IO_PORT |= (1 << o);
	} else {
		IO_PORT &= ~(1 << o);
	}
}

int io_read(Button b) { 
	return (IO_PIN >> b) & 1; 
}

#endif
