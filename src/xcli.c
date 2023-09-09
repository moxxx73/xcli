#include "../include/xcli.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/ioctl.h>

xcli_screen term_screen;
struct termios tsettings_save;

#define ENABLE_ALTBUF "\x1b[?1049h"
#define DISABLE_ALTBUF "\x1b[?1049l"

int cursor_pos(unsigned int col, unsigned int row){
	char buf[128];
	
	if(col > term_screen.cols || col == 0 || row > term_screen.rows || row == 0) return 0;
	memset(buf, 0, 128);

	snprintf(buf, 128, "\x1b[%u;%uH", row, col);
	return write(1, buf, strlen(buf));
}

#define CLR_SCREEN "\x1b[2J"
int clr_term(void){ return write(1, CLR_SCREEN, 4); }

#define HIDE_CURSOR "\x1b[?25l"
#define SHOW_CURSOR "\x1b[?25h"
int hide_cursor(void){
	if(write(1, HIDE_CURSOR, 6) != 6) return -1;
	return 0;
}

int show_cursor(void){
	if(write(1, SHOW_CURSOR, 6) != 6) return -1;
	return 0;
}

int get_termsize(unsigned int *cols, unsigned int *rows){
	struct winsize win;
	if(!rows || !cols) return -1;
	
	memset(&win, 0, sizeof(struct winsize));
	if(ioctl(1, TIOCGWINSZ, &win) == -1 || win.ws_col == 0) return -1;
	*rows = win.ws_row;
	*cols = win.ws_col;
	return 0;
}

void restore_term(void){
	xcli_window *winptr = NULL, *next = NULL;
	winptr = term_screen.windows.next_win;
	while(winptr){
		next = winptr->next_win;
		memset(winptr, 0, sizeof(xcli_window));
		free(winptr);
		winptr = next;
	}

	if(term_screen.sbuf) free(term_screen.sbuf);
	memset(&term_screen, 0, sizeof(xcli_screen));
	if(tcsetattr(0, TCSAFLUSH, &tsettings_save) == -1) return;
	if(write(1, DISABLE_ALTBUF, 8) != 8) return;
	return;
}

int init_term(void){
	struct termios new_settings;

	if(tcgetattr(0, &tsettings_save) == -1) return -1;
	memcpy(&new_settings, &tsettings_save, sizeof(struct termios));

	new_settings.c_iflag &= ~(IXON | ICRNL);
	new_settings.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	new_settings.c_oflag &= ~(OPOST);

	if(tcsetattr(0, TCSAFLUSH, &new_settings) == -1) return -1;
	if(write(1, ENABLE_ALTBUF, 8) != 8) return -1;
	
	if(get_termsize(&term_screen.cols, &term_screen.rows) == -1) return -1;
	term_screen.sbuf = (char *)malloc((term_screen.cols*term_screen.rows)+1);
	if(!term_screen.sbuf) return -1;

	memset(term_screen.sbuf, 0, (term_screen.cols*term_screen.rows)+1);
	term_screen.windows.start_col = 1;
	term_screen.windows.end_col = term_screen.cols;
	term_screen.windows.start_row = 1;
	term_screen.windows.end_row = term_screen.rows;

	return 0;
}

int new_window(int wid, unsigned int start_col, unsigned int start_row, unsigned int end_col, unsigned int end_row){
	xcli_window *winptr = NULL;
	winptr = &term_screen.windows;
	
	if(start_col > term_screen.cols || end_col > term_screen.cols || start_row > term_screen.rows || end_row > term_screen.rows) return -1;

	while(winptr->next_win) winptr = winptr->next_win;
	if(winptr->wid == INT_MAX) return -1;
	winptr->next_win = (xcli_window *)malloc(sizeof(xcli_window));

	if(!winptr->next_win) return -1;
	memset(winptr->next_win, 0, sizeof(xcli_window));
	winptr->next_win->prev_win = winptr;

	winptr->next_win->start_col = start_col;
	winptr->next_win->end_col = end_col;
	
	winptr->next_win->start_row = start_row;
	winptr->next_win->end_row = end_row;

	if(wid != 0 && wid != INT_MAX) winptr->next_win->wid = wid;
	else winptr->next_win->wid = winptr->wid+1;

	return winptr->next_win->wid;
}

void close_window(int wid){
	xcli_window *winptr = NULL;
	winptr = &term_screen.windows;

	if(wid == 0) return;
	
	while(winptr->next_win){
		if(winptr->wid == wid) break;
		winptr = winptr->next_win;
	}

	if(winptr->prev_win) winptr->prev_win->next_win = winptr->next_win;
	if(winptr->next_win) winptr->next_win->prev_win = winptr->prev_win;

	memset(winptr, 0, sizeof(xcli_window));
	free(winptr);
	return;
}

void write_string(int wid, char *str, unsigned long len, unsigned int col, unsigned int row){
	xcli_window *winptr = NULL;
	char *scrn_ptr = NULL;

	if(!term_screen.sbuf || !str || len == 0 || col >= term_screen.cols || row >= term_screen.rows) return;

	winptr = &term_screen.windows;
	while(winptr->next_win){
		if(winptr->wid == wid) break;
		winptr = winptr->next_win;
	}

	if(col >= winptr->start_col || col >= winptr->end_col || row >= winptr->start_row || row >= winptr->end_row) return;
	if(len >= (winptr->end_col-(winptr->start_col+col))) return;

	scrn_ptr = (term_screen.sbuf+(term_screen.cols*(winptr->start_row+row))+col);
	memcpy(scrn_ptr, str, len);
	return;
}

void fill_sbuf(char c){
	memset(term_screen.sbuf, c, (term_screen.cols*term_screen.rows));
	return;
}

void flush_sbuf(void){
	unsigned int cur_row = 1;
	if(!term_screen.sbuf) return;
	
	for(; cur_row < term_screen.rows; cur_row++){
		write(1, (term_screen.sbuf+(cur_row*term_screen.cols)), term_screen.cols);
		write(1, "\r\n", 2);
	}
	return;
}

char read_char(void){
	char key = 0;
	ssize_t r = 0;

	while((r = read(0, &key, 1)) != 1){
		if(r == -1 && errno != EAGAIN) return 0;
	}
	return key;
}
