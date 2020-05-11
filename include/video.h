#ifndef __VIDEO__
#define __VIDEO__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <alphabet.h>

/*
 *Display config
 */
#define LCD_PORT PORTB
#define LCD_DDR DDRB
#define LCD_DC_PIN PB1
#define LCD_CE_PIN PB2
#define SPI_MOSI_PIN PB3
#define LCD_RST_PIN PB4
#define SPI_CLK_PIN PB5
#define LCD_W 84
#define LCD_H 48
#define LCD_CACHE_SIZE ((LCD_W * LCD_H) / 8)

//#define __alternative__

/*
 *config end
 */

typedef unsigned char byte;

typedef enum {
	WHITE = 0,
	BLACK = 1,
	INVERT = 2
} Color;

byte LCD_CACHE[LCD_CACHE_SIZE];
int LWM;
int HWM;

void wait(void) {
	for (int i = -32000; i < 32000; i++);
}

void lcd_send_data(byte data) {
	LCD_PORT &= ~(1 << LCD_CE_PIN);
	LCD_PORT |= (1 << LCD_DC_PIN);
	SPDR = data;
	while ((SPSR & 0x80) != 0x80);
	LCD_PORT |= (1 << LCD_CE_PIN);
}

void lcd_send_cmd(byte data) {
	LCD_PORT &= ~(1 << LCD_CE_PIN);
	LCD_PORT &= ~(1 << LCD_DC_PIN);
	SPDR = data;
	while ((SPSR & 0x80) != 0x80);
	LCD_PORT |= (1 << LCD_CE_PIN);
}

void lcd_clear(void) {
	memset(LCD_CACHE, 0x00, LCD_CACHE_SIZE);
	LWM = 0;
	HWM = LCD_CACHE_SIZE - 1;
}

void lcd_update(void) {
	int i;
	if (LWM < 0) LWM = 0;
	else if (LWM >= LCD_CACHE_SIZE) LWM = LCD_CACHE_SIZE - 1;
	if (HWM < 0) HWM = 0;
	else if (HWM >= LCD_CACHE_SIZE) HWM = LCD_CACHE_SIZE - 1;

	#ifdef __alternative__

	byte x, y;
	x = LWM % LCD_W;
	lcd_send_cmd(0x80 | x);
	y = LWM / LCD_W + 1;
	lcd_send_cmd(0x40 | y);
	for (i = LWM; i <= HWM; i++) {
		lcd_send_data(LCD_CACHE[i]);
		x++;
		if (x >= LCD_W) {
			x = 0;
			lcd_send_cmd(0x80);
			y++;
			lcd_send_cmd(0x40 | y);
		}
	}

	lcd_send_cmd(0x21);
	lcd_send_cmd(0x45);
	lcd_send_cmd(0x20);

	#else

	lcd_send_cmd(0x80 | (LWM % LCD_W));
	lcd_send_cmd(0x40 | (LWM / LCD_W));
	for (i = LWM; i <= HWM; i++) {
		lcd_send_data(LCD_CACHE[i]);
	}

	#endif

	LWM = LCD_CACHE_SIZE - 1;
	HWM = 0;
}

void lcd_init(void) {
	LCD_PORT |= (1 << LCD_RST_PIN);
	LCD_DDR |= (1 << LCD_RST_PIN) | (1 << LCD_DC_PIN) | (1 << LCD_CE_PIN) | (1 << SPI_MOSI_PIN) | (1 << SPI_CLK_PIN);
	wait();
	LCD_PORT &= ~(1 << LCD_RST_PIN);
	wait();
	LCD_PORT |= (1 << LCD_RST_PIN);
	SPCR = 0x50;
	LCD_PORT |= (1 << LCD_CE_PIN);
	lcd_send_cmd(0x21);
	lcd_send_cmd(0xC8);
	lcd_send_cmd(0x06);
	lcd_send_cmd(0x13);
	lcd_send_cmd(0x20);
	lcd_send_cmd(0x0C);
	lcd_clear();
	lcd_update();
}

void lcd_pixel(byte x, byte y, Color c) {
	int index;
	byte offset, data;
	if (x >= LCD_W || y >= LCD_H) return;
	index = ((y / 8) *84) + x;
	offset = y - ((y / 8) *8);
	data = LCD_CACHE[index];
	if (c == BLACK) {
		data |= (0x01 << offset);
	} 
	if (c == WHITE) {
		data &= (~(0x01 << offset));
	}
	if (c == INVERT) {
        data ^= (0x01 << offset);
    }

	LCD_CACHE[index] = data;
	if (index < LWM) {
		LWM = index;
	}

	if (index > HWM) {
		HWM = index;
	}
}

int min(int a, int b) {
	if (a > b) return b;
	return a;
}

int max(int a, int b) {
	if (a > b) return a;
	return b;
}

void lcd_h_line(byte x0, byte y0, byte x1, Color c) {
	for (int i = min(x0, x1); i <= max(x0, x1); i++) lcd_pixel(i, y0, c);
}

void lcd_v_line(byte x0, byte y0, byte y1, Color c) {
	for (int i = min(y0, y1); i <= max(y0, y1); i++) lcd_pixel(x0, i, c);
}

void lcd_line(byte x1, byte y1, byte x2, byte y2, Color c) {
	if (x1 == x2) {
		lcd_v_line(x1, y1, y2, c);
		return;
	}

	if (y1 == y2) {
		lcd_h_line(x1, y1, x2, c);
		return;
	}
	int dx, dy, stepx, stepy, fraction;
	dy = y2 - y1;
	dx = x2 - x1;
	if (dy < 0) {
		dy = -dy;
		stepy = -1;
	} else {
		stepy = 1;
	}
	if (dx < 0) {
		dx = -dx;
		stepx = -1;
	} else {
		stepx = 1;
	}
	dx <<= 1;
	dy <<= 1;
	if (dx > dy) {
		fraction = dy - (dx >> 1);
		while (x1 != x2) {
			if (fraction >= 0) {
				y1 += stepy;
				fraction -= dx;
			}
			x1 += stepx;
			fraction += dy;
			lcd_pixel(x1, y1, c);
		}
	} else {
		fraction = dx - (dy >> 1);
		while (y1 != y2) {
			if (fraction >= 0) {
				x1 += stepx;
				fraction -= dy;
			}
			y1 += stepy;
			fraction += dx;
			lcd_pixel(x1, y1, c);
		}
	}
}

void lcd_put_char(byte x0, byte y0, unsigned char ch, Color c) {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 8; j++) {
			if ((eeprom_read_byte(&CHARS_TABLE[(ch - 32) * 5 + i]) >> j) & 1) lcd_pixel(x0 + i, y0 + j, c);
		}
	}
}

int lcd_put_string(byte x0, byte y0, char *s, Color c) {
	int chars_printed = strlen(s);
	for (int i = 0; i< chars_printed; i++) {
		lcd_put_char(x0 + 6 * i, y0, (unsigned char) s[i], c);
	}
	return chars_printed;
}

void lcd_draw(const byte *texture, byte x, byte y, byte w, byte h, Color c) {
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			char addr = j * w + i; 
			if (eeprom_read_byte(&texture[addr / 8]) >> (addr % 8) & 1) lcd_pixel(x + i, y + j, c);
		}
	}
}

# endif