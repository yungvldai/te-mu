#ifndef __TEXTURES__
#define __TEXTURES__

#include <avr/eeprom.h>

const byte small_zero[] EEMEM = {
	0b10010110,
	0b10011001,
	0b110
};

const byte small_two[] EEMEM = {
	0b11100111,
	0b1110011
};

const byte small_four[] EEMEM = {
	0b11101101, 
	0b1001001
};

const byte small_one[] EEMEM = {
	0b10101110, 
	0b10
};

const byte small_eight[] EEMEM = {
	0b11101111, 
	0b1111011
};

const byte small_s[] EEMEM = {
	0b11001111, 
	0b1111001
};

const byte small_p[] EEMEM = {
	0b11101111, 
	0b0010011
};

			   
const byte ball[] EEMEM = {
	0b111100,
	0b1111110,
	0b11111111,
	0b10111111,
	0b10111111,
	0b11011111,
	0b1111110,
	0b111100
};
			   
const byte deg_char[] EEMEM = {
	0b1001110,
	0b111001
};


#endif