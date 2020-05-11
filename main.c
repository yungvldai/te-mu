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
void draw_hook_ptr(char, unsigned char);

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
	set = 1,
	res = 2,
	nop = 0
} Action;

typedef enum {
	eq = 0,
	lt = 1,
	gt = 2,
	lte = 3,
	gte = 4
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
char hook_edit_stage = 0;

char port_status[PORT_SIZE] = {0, 0, 0, 0};

void hooks_init(void) {
	for (int i = 0; i < HOOKS_N; i++) {
		hooks[i].port = Z;
	}
}

void clear_port(void) {
	for (int i = 0; i < PORT_SIZE; i++) port_status[i] = 0;
}

void apply_port(void) {
	for (int i = 0; i < PORT_SIZE; i++) io_write(i + 4, port_status[i]);
}

void print_hook(int);

void do_action(Port prt, Action ac) {
	if (ac == set) port_status[prt - 4] = 1;
	if (ac == res) port_status[prt - 4] = 0;
}

void handle_hooks(int t_value) {
	int value = 0;
	if (t_sign(t_value)){
		value = -t_integer_part(~t_value);
	} else {
		value = t_integer_part(t_value);
	}

	clear_port();
	for (int i = 0; i < HOOKS_N; i++) {
		hook th = hooks[i];
		
		char cond = (th.op == eq && value == th.value) || 
					(th.op == lt && value < th.value) ||
					(th.op == gt && value > th.value) ||
					(th.op == lte && value <= th.value) ||
					(th.op == gte && value >= th.value);
		
		if (cond) {
			do_action(th.port, th.act);
		}
	}
	apply_port();
} 

int main(void) {
	lcd_init();
	io_init();
	clock_init();
	hooks_init();
	
	hooks[0].port = X;
	hooks[0].act = set;
	hooks[0].op = eq;
	hooks[0].value = 24;

	while (1) {
		_delay_ms(10);
		
		handle_hooks(shown_value);
		
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
			draw_current_value(shown_value);
			draw_chart(ltp);
		}
		
		if (vm == HOOKS) {
			lcd_put_string(1, 1, "Hooks", BLACK);
			for (int i = 0; i < HOOKS_N; i++) {
				print_hook(i);
			}
			
			if (io_read(A) && clock() - debounce_a > DEBOUNCE_T) {
				if (hook_edit_stage == 0) {
					if (hook_m_ptr > 0) hook_m_ptr--;
				}
				if (hook_edit_stage == 1) {
					if (hooks[hook_m_ptr].op < 4) hooks[hook_m_ptr].op++;
				}
				if (hook_edit_stage == 2) {
					if (hooks[hook_m_ptr].value < 99) hooks[hook_m_ptr].value++;
				}
				if (hook_edit_stage == 3) {
					if (hooks[hook_m_ptr].act < 2) hooks[hook_m_ptr].act++;
				}
				if (hook_edit_stage == 4) {
					if (hooks[hook_m_ptr].port < W) hooks[hook_m_ptr].port++;
				}
				debounce_a = clock();
			}
			if (io_read(B) && clock() - debounce_b > DEBOUNCE_T) {
				if (hook_edit_stage == 0) {
					if (hook_m_ptr < 3) hook_m_ptr++;
				}
				if (hook_edit_stage == 1) {
					if (hooks[hook_m_ptr].op > 0) hooks[hook_m_ptr].op--;
				}
				if (hook_edit_stage == 2) {
					if (hooks[hook_m_ptr].value > -55) hooks[hook_m_ptr].value--;
				}
				if (hook_edit_stage == 3) {
					if (hooks[hook_m_ptr].act > 0) hooks[hook_m_ptr].act--;
				}
				if (hook_edit_stage == 4) {
					if (hooks[hook_m_ptr].port > Z) hooks[hook_m_ptr].port--;
				}
				debounce_b = clock();
			}
			if (io_read(C) && clock() - debounce_c > DEBOUNCE_T) {
				hook_edit_stage++;
				if ((hook_edit_stage > 3 && hooks[hook_m_ptr].act == nop) || (hook_edit_stage > 4 && hooks[hook_m_ptr].act != nop)) hook_edit_stage = 0;
				debounce_c = clock();
			}
			
			draw_hook_ptr(hook_m_ptr + 1, hook_edit_stage);
		}
		
		lcd_update();
	}

	return 0;
}

const unsigned char* edit_stages[][2] = {
	{ 0, 83 },
	{ 0, 13 }, 
	{ 17, 36 },
	{ 41, 60 },
	{ 67, 73 }
};

void draw_hook_ptr(char ptr, unsigned char stage) {
	for (int i = 2; i < 11; i++) {
		lcd_line(edit_stages[stage][0], ptr * 9 + i, edit_stages[stage][1], ptr * 9 + i, INVERT);
	}
}

char* cmps[] = {"=", "<", ">", "<=", ">="};
char* acts[] = {"nop", "set", "res"};
char* ports[] = {"Z", "Y", "X", "W"};

void print_hook(int index) {
	byte py = (index + 1) * 9 + 3;
	lcd_put_string(1, py, cmps[hooks[index].op], BLACK);
	int value = hooks[index].value;
	char buf[5];
	itoa(value, buf, 10);
	lcd_put_string(18, py, buf, BLACK);
	lcd_put_string(42, py, acts[hooks[index].act], BLACK);
	if (hooks[index].act != nop) {
		lcd_put_string(68, py, ports[hooks[index].port - 4], BLACK);
	}
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
	chars_printed = lcd_put_string(lcd_print_ptr, py, "0", BLACK);
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