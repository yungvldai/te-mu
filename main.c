#include <video.h>
#include <textures.h>
#include <dt.h>
#include <io.h>
#include <clock.h>

#define CHART_W 57
#define ZERO_H 12

#define DEBOUNCE_T 250
#define CYCLE_CORRECTION 100
#define HOOKS_N 4

typedef enum {
	MAIN = 0,
	HOOKS = 1
} ViewMode;

typedef enum {
	MAX20 = 20, 
	MAX40 = 40, 
	MAX80 = 80
} LineType;

typedef enum {
	X1 = 3600, 
	X2 = 60, 
	X3 = 1
} UpdateSpeed;

int values_for_chart[CHART_W];

void draw_interface(void);
void draw_values_line(LineType);
void draw_update_speed_indicator(UpdateSpeed);
void draw_current_value(int);
void draw_chart(LineType);
void draw_bar(LineType, int);
void draw_hook_ptr(char);

void clear_chart(void);
void add_new_chart_value(int);

timestamp debounce_a = 0;
timestamp debounce_b = 0;
timestamp debounce_c = 0;
timestamp debounce_d = 0;

UpdateSpeed usp = X3;
void change_upd_speed(void) {
	if (usp == X3) {
		usp = X1;
		return;
	}
	if (usp == X2) {
		usp = X3;
		return;
	}
	if (usp == X1) {
		usp = X2;
		return;
	}
}

LineType ltp = MAX40;
void change_line_type(void) {
	if (ltp == MAX40) {
		ltp = MAX80;
		return;
	}
	if (ltp == MAX80) {
		ltp = MAX20;
		return;
	}
	if (ltp == MAX20) {
		ltp = MAX40;
		return;
	}
}

timestamp conversion_start = 0;
char conversion = 0;

char ovf_flag = 0;
int actual_t = 0;
int shown_value = 0;
timestamp last_upd = 0;

ViewMode vm = MAIN;
void change_view_mode(void) {
	if (vm == MAIN) {
		vm = HOOKS;
		return;
	}
	if (vm == HOOKS) {
		vm = MAIN;
		return;
	}
}

typedef enum {
	set = 0,
	res = 1,
	tog = 2,
	sal = 3,
	ral = 4,
	tal = 5,
	nop = 6
} Action;

typedef enum {
	eq = 1,
	lt = 2,
	gt = 3,
	lte = 4,
	gte = 5
} CmpOperator;

typedef struct {
	CmpOperator op;
	// if
	int value; 
	// do
	Action act;
	// on
	Port port; 
} hook;

hook hooks[HOOKS_N];
char hook_m_ptr = 0;

void hooks_init(void) {
	for (int i = 0; i < HOOKS_N; i++) {
		hooks[i].act = nop;
		hooks[i].port = Z;
		hooks[i].op = eq;
	}
}

void print_hook(int);

int main(void) {
	lcd_init();
	io_init();
	clock_init();
	hooks_init();

	while (1) {
		_delay_ms(10);
		
		/* 
		 * Non-blocking temp conversion
		 */
		 
		if (!conversion) {
			dt_convert();
			conversion = 1;
			conversion_start = clock();
		}
		
		if (clock() - conversion_start > CONV_TIME) {
			actual_t = dt_read();
			conversion = 0;
		}
		
		/*
		 * Value update with chosen period
		 */
		
		if (clock() - last_upd >= ((usp * 1000) - CYCLE_CORRECTION)) {
			shown_value = actual_t;
 			add_new_chart_value(shown_value);
			last_upd = clock();
		}
		
		lcd_clear();
		
		if (io_read(D) && clock() - debounce_d > DEBOUNCE_T) {
			change_view_mode();
			debounce_d = clock();
		}
		
		if (vm == MAIN) {

			/*
			 * Handle buttons
			 */

			if (io_read(A) && clock() - debounce_a > DEBOUNCE_T) {
				change_upd_speed();
				debounce_a = clock();
			}
			if (io_read(B) && clock() - debounce_b > DEBOUNCE_T) {
				change_line_type();
				debounce_b = clock();
			}
			draw_interface();
			draw_values_line(ltp);
			draw_update_speed_indicator(usp);
			draw_bar(ltp, shown_value);
			//draw_current_value(shown_value);
			//draw_chart(ltp);
			
			lcd_draw_v2(_tball, 10, 10, 8, 8, BLACK);
		}
		
		if (vm == HOOKS) {
			lcd_put_string(1, 1, "Hooks", BLACK);
			for (int i = 0; i < HOOKS_N; i++) {
				print_hook(i);
			}
			
			if (io_read(A) && clock() - debounce_a > DEBOUNCE_T) {
				if (hook_m_ptr > 0) hook_m_ptr--;
				debounce_a = clock();
			}
			if (io_read(B) && clock() - debounce_b > DEBOUNCE_T) {
				if (hook_m_ptr < 3) hook_m_ptr++;
				debounce_b = clock();
			}
			
			draw_hook_ptr(hook_m_ptr + 1);
		}
		
		lcd_update();
	}

	return 0;
}

void draw_hook_ptr(char ptr) {
	for (int i = 2; i < 11; i++) {
		lcd_line(0, ptr * 9 + i, 83, ptr * 9 + i, INVERT);
	}
}

void print_hook(int index) {
/*
	byte py = (index + 1) * 9 + 3;	
	
	if (hooks[index].op == eq) {
		lcd_put_string(1, py, "=", BLACK);
	}
	if (hooks[index].op == lt) {
		lcd_put_string(1, py, "<", BLACK);
	}
	if (hooks[index].op == gt) {
		lcd_put_string(1, py, ">", BLACK);
	}
	if (hooks[index].op == lte) {
		lcd_put_string(1, py, "<=", BLACK);
	}
	if (hooks[index].op == gte) {
		lcd_put_string(1, py, ">=", BLACK);
	}
	
	int value = hooks[index].value;
	char buf[5];
	itoa(value, buf, 10);
	lcd_put_string(18, py, buf, BLACK);
	if (hooks[index].act == set) {
		lcd_put_string(42, py, "set", BLACK);
	}
	if (hooks[index].act == res) {
		lcd_put_string(42, py, "res", BLACK);
	}
	if (hooks[index].act == tog) {
		lcd_put_string(42, py, "tog", BLACK);
	}
	if (hooks[index].act == sal) {
		lcd_put_string(42, py, "sal", BLACK);
	}
	if (hooks[index].act == ral) {
		lcd_put_string(42, py, "ral", BLACK);
	}
	if (hooks[index].act == tal) {
		lcd_put_string(42, py, "tal", BLACK);
	}
	if (hooks[index].act == nop) {
		lcd_put_string(42, py, "nop", BLACK);
	}
	
	if (hooks[index].port == Z) {
		lcd_put_char(68, py, 'Z', BLACK);
	}
	if (hooks[index].port == Y) {
		lcd_put_char(68, py, 'Y', BLACK);
	}
	if (hooks[index].port == X) {
		lcd_put_char(68, py, 'X', BLACK);
	}
	if (hooks[index].port == W) {
		lcd_put_char(68, py, 'W', BLACK);
	}
	*/

}

int sc_top(LineType lt, int value) {
	int divs = 0;
	if (t_sign(value)) {
		divs = -(t_integer_part(~value) * 16 / lt);
	} else {
		divs = t_integer_part(value) * 16 / lt;
	}
	int scale_top = 38 - (divs + ZERO_H);
	ovf_flag = 0;
	if (scale_top < 2) {
		scale_top = 2;
		ovf_flag = 1;
	}
	if (scale_top > 37) {
		scale_top = 37;
		ovf_flag = 1;
	}
	return scale_top;
}

void draw_chart(LineType lt) {
	for (byte i = 0; i < CHART_W - 1; i++) {
		lcd_line(
			i + 2, sc_top(lt, values_for_chart[i]), 
			i + 3, sc_top(lt, values_for_chart[i + 1]), 
			BLACK
		);
	}
}

void add_new_chart_value(int new_value) {
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

void draw_bar(LineType lt, int value) {
	byte scale_top = sc_top(lt, value);
	lcd_line(78, scale_top, 78, 37, BLACK);
	lcd_line(79, scale_top, 79, 37, BLACK);
}
 
void draw_current_value(int value) {
	char buf[5];
	const char py = 40;
	char lcd_print_ptr = 1;
	int work_value = value;
	char is_negative = t_sign(work_value);
	if (is_negative) {
		lcd_put_char(lcd_print_ptr, py, '-', BLACK);
		lcd_print_ptr += 6;
		work_value = ~work_value;
	}
	itoa(t_integer_part(work_value), buf, 10);
	char chars_printed = lcd_put_string(lcd_print_ptr, py, buf, BLACK);
	lcd_print_ptr += (6 * chars_printed);
	lcd_put_char(lcd_print_ptr, py, '.', BLACK);
	lcd_print_ptr += 6;
	itoa((int)(t_float_part(value) * 10.0), buf, 10);
	chars_printed = lcd_put_string(lcd_print_ptr, py, buf, BLACK);
	lcd_print_ptr += (6 * chars_printed);
	lcd_draw(deg_char, lcd_print_ptr, py, 5, 3, BLACK);
	lcd_print_ptr += 6;
	lcd_put_char(lcd_print_ptr, py, 'C', BLACK);
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
			lcd_draw(small_one, 66, 16, 2, 5, BLACK);
			lcd_draw(small_one, 66, 32, 2, 5, BLACK);
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
		lcd_pixel(76, 2 + (4 *i), BLACK);
		if (i % 2 == 0) lcd_pixel(75, 2 + (4 *i), BLACK);
	}
	for (int i = 1; i <= 4; i++) {
		lcd_draw(small_zero, 70, 8 *i, 4, 5, BLACK);
	}
	lcd_line(62, 34, 64, 34, BLACK);
	lcd_draw(ball, 75, 39, 8, 8, BLACK);
	lcd_draw(small_s, 54, 41, 3, 5, BLACK);
	lcd_draw(small_p, 58, 41, 3, 5, BLACK);
	if (ovf_flag) {
		for (int i = 66; i <= 70; i++) {
			lcd_line(i, 1, i, 6, BLACK);
		}
		lcd_line(68, 2, 68, 5, WHITE);
		lcd_pixel(68, 4, BLACK);
	}
}