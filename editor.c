#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

int x, y;

WINDOW* win;

int ln_end() {
	int mx, my;
	getmaxyx(win, my, mx);
	int lx;
	lx = mx;
	while ((inch() & A_CHARTEXT) == ' ') {
		lx = mx--;
		move(y, lx);
		if (lx == -1) {
			break;
		}
	}
	lx++;
    return lx;
}

void mvln(int count) {
	y += count;
    x = ln_end();
	move(y, x);
}


void handle_key(int key) {
	char ch = (char)key;
	if (isprint(key)) {
		addch(ch);
		x++;
	} else if(key == '\n') {
		addch('\n');
		y++;
		x = 0;
	} else if(key == KEY_BACKSPACE) {
		if (x > 0 ) {
			x--;
			move(y, x);
			delch();
		} else if(y > 0) {
			mvln(-1);
		}
	} else if(key == KEY_LEFT) {
		if (x > 0) {
            x--;
            move(y, x);
		} else if (y > 0) {
            mvln(-1);
            printw("HERE", 100, 100);
        }
	} else if (key == KEY_UP) {
    	//	if (x++ == 
	} else if (key == KEY_DOWN) {
		
    } else {
		
	}
}

int main() {
	win = initscr();
	noecho();
	
	keypad(win, true);

	while(true) {
		move(y, x);
		handle_key(getch());
		refresh();
	}
	endwin();

	return 0;
}
