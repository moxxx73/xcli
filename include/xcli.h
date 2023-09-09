#ifndef XCLI_H
#define XCLI_H

typedef struct _xcli_window{
	struct _xcli_window *prev_win;
	int wid;
	unsigned int start_col;
	unsigned int end_col;
	unsigned int start_row;
	unsigned int end_row;
	struct _xcli_window *next_win;
} xcli_window;

typedef struct{
	unsigned int cols;
	unsigned int rows;
	xcli_window windows;	
	char *sbuf;
} xcli_screen;

extern xcli_screen term_screen;

int cursor_pos(unsigned int col, unsigned int row);

int clr_term(void);

int hide_cursor(void);
int show_cursor(void);

int get_termsize(unsigned int *cols, unsigned int *rows);

void restore_term(void);

int init_term(void);

int new_window(int wid, unsigned int start_col, unsigned int start_row, unsigned int end_col, unsigned int end_row);

void close_window(int wid);

void write_string(int wid, char *str, unsigned long len, unsigned int col, unsigned int row);

void fill_sbuf(char c);

void flush_sbuf(void);

char read_char(void);

#endif
