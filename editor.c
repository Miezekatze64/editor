#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

int pos = 0;

WINDOW* win;

char text[10];

void add(char c) {
/*    memmove(
        text + pos;
    );
*/
}

void setcursor() {
    
}

void del() {
    
}

void handle_key(int key) {
	char ch = (char)key;
    
	if (isprint(key)) {
		add(ch);
	} else if(key == '\n') {
		add('\n');
	} else if(key == KEY_BACKSPACE) {
        del();
	} else if(key == KEY_LEFT) {
        pos--;
    } else if(key == KEY_RIGHT) {
       pos++;
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
		setcursor();
		handle_key(getch());
		refresh();
	}
	endwin();

	return 0;
}
