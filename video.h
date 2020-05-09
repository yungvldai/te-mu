#ifndef __VIDEO__
#define __VIDEO__

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <alphabet.h>

/*
 * Display config
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
 * config end
 */

typedef unsigned char byte;
typedef enum { WHITE = 0, BLACK = 1 } Color;

byte LCD_CACHE[LCD_CACHE_SIZE], LCD_UPD_FLAG;
int LWM, HWM;

void wait(void) {
  for (int i = -32000; i < 32000; i++);
}

void lcd_send_data(byte data) {
  LCD_PORT &= ~(_BV(LCD_CE_PIN));
  LCD_PORT |= _BV(LCD_DC_PIN);
  SPDR = data;
  while ((SPSR & 0x80) != 0x80);
  LCD_PORT |= _BV(LCD_CE_PIN);
}

void lcd_send_cmd(byte data) {
  LCD_PORT &= ~(_BV(LCD_CE_PIN));
  LCD_PORT &= ~(_BV(LCD_DC_PIN));
  SPDR = data;
  while ((SPSR & 0x80) != 0x80)
    ;
  LCD_PORT |= _BV(LCD_CE_PIN);
}

void lcd_clear(void) {
  memset(LCD_CACHE, 0x00, LCD_CACHE_SIZE);
  LWM = 0;
  HWM = LCD_CACHE_SIZE - 1;
  LCD_UPD_FLAG = 1;
}

void lcd_update(void) {
  int i;
  if (LWM < 0)
    LWM = 0;
  else if (LWM >= LCD_CACHE_SIZE)
    LWM = LCD_CACHE_SIZE - 1;
  if (HWM < 0)
    HWM = 0;
  else if (HWM >= LCD_CACHE_SIZE)
    HWM = LCD_CACHE_SIZE - 1;

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
  LCD_UPD_FLAG = 0;
}

void lcd_init(void) {
  LCD_PORT |= _BV(LCD_RST_PIN);
  LCD_DDR |= _BV(LCD_RST_PIN) | _BV(LCD_DC_PIN) | _BV(LCD_CE_PIN) |
             _BV(SPI_MOSI_PIN) | _BV(SPI_CLK_PIN);
  wait();
  LCD_PORT &= ~(_BV(LCD_RST_PIN));
  wait();
  LCD_PORT |= _BV(LCD_RST_PIN);
  SPCR = 0x50;
  LCD_PORT |= _BV(LCD_CE_PIN);
  lcd_send_cmd(0x21);
  lcd_send_cmd(0xC8);
  lcd_send_cmd(0x06);
  lcd_send_cmd(0x13);
  lcd_send_cmd(0x20);
  lcd_send_cmd(0x0C);
  lcd_clear();
  lcd_update();
}

void lcd_contrast(byte contrast) {
  lcd_send_cmd(0x21);
  lcd_send_cmd(0x80 | contrast);
  lcd_send_cmd(0x20);
}

void lcd_pixel(byte x, byte y, Color c) {
  int index;
  byte offset, data;
  if (x >= LCD_W || y >= LCD_H) return;
  index = ((y / 8) * 84) + x;
  offset = y - ((y / 8) * 8);
  data = LCD_CACHE[index];
  if (c == BLACK) {
    data |= (0x01 << offset);
  } else {
    data &= (~(0x01 << offset));
  }
  LCD_CACHE[index] = data;
  if (index < LWM) {
    LWM = index;
  }
  if (index > HWM) {
    HWM = index;
  }
}

void lcd_h_line(byte x0, byte y0, byte x1, Color c) {
  for (int i = x0; i <= x1; i++) lcd_pixel(i, y0, c);
}

void lcd_v_line(byte x0, byte y0, byte y1, Color c) {
  for (int i = y0; i <= y1; i++) lcd_pixel(x0, i, c);
}

void lcd_line(byte x0, byte y0, byte x1, byte y1, Color c) {
  if (x0 == x1) {
    lcd_v_line(x0, y0, y1, c);
    return;
  }
  if (y0 == y1) {
    lcd_h_line(x0, y0, x1, c);
    return;
  }
  int dx = abs(x1 - x0), sx = -1;
  if (x0 < x1) sx = 1;
  int dy = abs(y1 - y0), sy = -1;
  if (y0 < y1) sy = 1;
  int err = dy / -2, e2;
  if (dx > dy) err = dx / 2;
  while (x0 != x1 && y0 != y1) {
    lcd_pixel(x0, y0, c);
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

void lcd_circle(byte x0, byte y0, byte r, Color c) {
  int f = 1 - r;
  int ddF_x = 1, ddF_y = -2 * r;
  int x = 0, y = r;
  lcd_pixel(x0, y0 + r, c);
  lcd_pixel(x0, y0 - r, c);
  lcd_pixel(x0 + r, y0, c);
  lcd_pixel(x0 - r, y0, c);
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    lcd_pixel(x0 + x, y0 + y, c);
    lcd_pixel(x0 - x, y0 + y, c);
    lcd_pixel(x0 + x, y0 - y, c);
    lcd_pixel(x0 - x, y0 - y, c);
    lcd_pixel(x0 + y, y0 + x, c);
    lcd_pixel(x0 - y, y0 + x, c);
    lcd_pixel(x0 + y, y0 - x, c);
    lcd_pixel(x0 - y, y0 - x, c);
  }
}

void lcd_put_char(byte x0, byte y0, unsigned char ch, Color c) {
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 8; j++) {
      if ((pgm_read_byte(&CHARS_TABLE[(ch - 32) * 5 + i]) >> j) & 1)
        lcd_pixel(x0 + i, y0 + j, c);
    }
  }
}

void lcd_put_string(byte x0, byte y0, char* s, Color c) {
  for (int i = 0; i < strlen(s); i++) {
    lcd_put_char(x0 + 6 * i, y0, (unsigned char)s[i], c);
  }
}

void lcd_draw(byte* texture, byte x, byte y, byte w, byte h, Color c) {
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      if (texture[j * w + i] & 1) lcd_pixel(x + i, y + j, c);
    }
  }
}

#endif