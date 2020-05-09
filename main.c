#include <video.h>
#include <textures.h>
#include <dt.h>

#define CHART_W 57
#define CHART_MIN -11
#define CHART_MAX 24

typedef enum { MAX20 = 20, MAX40 = 40, MAX80 = 80 } LineType;

typedef enum { X1 = 0, X2 = 1, X3 = 2 } UpdateSpeed;

byte values_for_chart[CHART_W];

void draw_interface(void);
void draw_values_line(LineType);
void draw_update_speed_indicator(UpdateSpeed);
void clear_chart(void);
void add_new_chart_value(byte);

int main(void) {
  lcd_init();

  lcd_clear();

  draw_interface();
  // draw_values_line(MAX40);
  // draw_update_speed_indicator(X2);

  lcd_update();

  while (1) {
    lcd_clear();

    int g = dt_get() >> 4;

    lcd_put_char(10, 10, g / 10 + 0x30, BLACK);
    lcd_put_char(16, 10, g % 10 + 0x30, BLACK);

    lcd_update();
  }

  return 0;
}

void draw_chart(LineType lt) {}

void add_new_chart_value(byte new_value) {
  for (int i = 1; i < CHART_W; i++) {
    values_for_chart[i - 1] = values_for_chart[i];
  }
  values_for_chart[CHART_W - 1] = new_value;
}

void clear_chart() {
  for (int i = 0; i < CHART_W; i++) {
    values_for_chart[i] = 0;
  }
}

void draw_update_speed_indicator(UpdateSpeed us) {
  lcd_line(62, 43, 62, 45, BLACK);
  lcd_line(63, 43, 63, 45, BLACK);
  if (us == X1) return;
  lcd_line(65, 42, 65, 45, BLACK);
  lcd_line(66, 42, 66, 45, BLACK);
  if (us == X2) return;
  lcd_line(68, 41, 68, 45, BLACK);
  lcd_line(69, 41, 69, 45, BLACK);
}

void draw_values_line(LineType lt) {
  switch (lt) {
    case MAX20:
      lcd_draw(small_two, 66, 8, 3, 5, BLACK);
      lcd_draw(small_one, 66, 16, 3, 5, BLACK);
      lcd_draw(small_one, 66, 32, 3, 5, BLACK);
      break;
    case MAX80:
      lcd_draw(small_eight, 66, 8, 3, 5, BLACK);
      lcd_draw(small_four, 66, 16, 3, 5, BLACK);
      lcd_draw(small_four, 66, 32, 3, 5, BLACK);
      break;
    case MAX40:
    default:
      lcd_draw(small_four, 66, 8, 3, 5, BLACK);
      lcd_draw(small_two, 66, 16, 3, 5, BLACK);
      lcd_draw(small_two, 66, 32, 3, 5, BLACK);
  }
}

void draw_interface() {
  lcd_line(1, 1, 59, 1, BLACK);
  lcd_line(1, 38, 59, 38, BLACK);
  lcd_line(1, 1, 1, 38, BLACK);
  lcd_line(59, 1, 59, 38, BLACK);
  for (int i = 1; i <= 59; i++) {
    if (i % 2 == 1) lcd_pixel(i, 26, BLACK);
  }
  lcd_line(77, 2, 77, 38, BLACK);
  lcd_line(80, 2, 80, 38, BLACK);
  lcd_line(78, 1, 79, 1, BLACK);
  lcd_line(78, 38, 79, 38, BLACK);
  for (int i = 1; i <= 8; i++) {
    lcd_pixel(76, 2 + (4 * i), BLACK);
    if (i % 2 == 0) lcd_pixel(75, 2 + (4 * i), BLACK);
  }
  for (int i = 1; i <= 4; i++) {
    lcd_draw(small_zero, 70, 8 * i, 4, 5, BLACK);
  }
  lcd_line(62, 34, 64, 34, BLACK);
  lcd_line(72, 40, 72, 46, BLACK);
  lcd_line(51, 40, 51, 46, BLACK);
  lcd_draw(ball, 75, 39, 8, 8, BLACK);
  lcd_draw(small_s, 54, 41, 3, 5, BLACK);
  lcd_draw(small_p, 58, 41, 3, 5, BLACK);
}
