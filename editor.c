#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>


#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif


int pos = 0;
int line_off = 0;

volatile sig_atomic_t stop;
WINDOW* win;

FILE *file;
char *filename;
char *message;
bool hasfile = false;

char *text;

void add(char c);
void setcursor();
void del();
void handle_key(int key);
char* getText();
void escape();
int get_offset();
void mv_line(size_t count);
void setempty();
int *cursorpos();

int main(int argc, char **argv) {
	if (argc > 1) {
		if (argv[1][0] != '-') {
			//parse as file
			char* str = argv[1];
			
			filename = malloc(strlen(str)+1);
			memcpy(filename, str, strlen(str)+1);
			
			if(access( filename, F_OK ) == 0) {
				file = fopen(str, "rwb");
				fseek(file, 0, SEEK_END);
				long fsize = ftell(file);
				fseek(file, 0, SEEK_SET);
				
				text = malloc(fsize + 1);
				fread(text, fsize, 1, file);
				fclose(file);
				text[fsize] = '\0';
	
				hasfile = true;
			}
		} else {
			printf("Argument %s not found", argv[0]);
		}
	}

    if (!hasfile) {
		text = malloc(1);
		text[0] = '\0';
	}
		
	win = initscr();
	noecho();
	raw();
	set_tabsize(4);
	keypad(win, true);
	
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);	//standard text
    init_pair(2, COLOR_GREEN, COLOR_BLACK);	//messages at screen bottom
    init_pair(3, COLOR_RED, COLOR_BLACK);	//end of file
	
	while(!stop) {
		erase();
		attron(COLOR_PAIR(3));
		attron(A_BOLD);
		setempty();
		attron(COLOR_PAIR(1));
		attroff(A_BOLD);
		
		char *ptr = getText();
		printw("%s", ptr, 0, 0);
		free(ptr);
		
		int y, x;
		x++;
		getmaxyx(win, y, x);
		if (message != NULL) {
			move(y-1, 0);
			clrtoeol();
			attron(COLOR_PAIR(2));
			mvprintw(y-1, 0, "%s", message);
			message = NULL;
		}
		
		setcursor();
		handle_key(getch());
		refresh();
	}
	clear();
	endwin();
	return 0;
}

void setempty() {
	int x, y;
	getmaxyx(win, y, x);
	x++; //avoid unused warnings...
	for (int i = 0; i < y; i++) {
		if (cursorpos()[0] < i) mvprintw(i, 0, "%s", "~");
	}
	move(0, 0);
}

char *getText() {
	size_t off = get_offset();
	char *t = malloc(strlen(text)-off+1);
	memcpy(t, text+off, strlen(text)-off+1);
	
	return t;
}

void add(char c) {
	char *new_array = malloc(strlen(text)+3 * sizeof(char));
	new_array[strlen(text)+2] = '\0';
	
	for (int i = 0; i < strlen(new_array)+1; i++) {
		if (i < pos) {
			new_array[i] = text[i];
		} else if (i > pos) {
			new_array[i] = text[i-1];
		} else {
			new_array[i] = c;
		}
	}
	
	free(text);
	
	text = new_array;
	pos++;
}

int *cursorpos() {
	int x = 0;
	int y = 0;
	int i;
	for (i = get_offset(); i < pos; i++) {
		if (text[i] == '\n') {
			y++;
			x = 0;
		} else {
			if (text[i] == '\t')
				x+= 4;
			else
				x++;
		}
	}
	int *rt = malloc(2);
	rt[0] = y;
	rt[1] = x;
	return rt;
}

void setcursor() {
	int *pos = cursorpos();
	move(pos[0], pos[1]);
}

void del() {
	if (pos <= 0) return;
	char *new_array = malloc((strlen(text)) * sizeof(char));

	for (int i = 0; i < strlen(new_array)+1; i++) {
		if (i < pos-1) {
			new_array[i] = text[i];
		} else if (i >= pos-1) {
			new_array[i] = text[i+1];
		}
	}

	new_array[strlen(new_array)] = '\0';
	free(text);
	text = new_array;
	pos--;
}

void up() {
	int chars = 0;
	if (pos > 0) {
		do {
			pos--;
			chars++;
		} while (pos > 0 && text[pos] != '\n');
		pos--;
		while (pos > 0 && text[pos] != '\n') {
			pos--;
		}
		if (pos == 0) chars--;
		
		for (int i = 0; i < chars; i++) {
			pos++;
			if (text[pos] == '\n') break;
		} 
	}
	if (cursorpos()[0] <= 0) mv_line(-1);
}

void down() {
	int chars = 0;
	if (pos <= strlen(text))  {
		do {
			pos--;
			chars++;
		} while (pos >= 0 && text[pos] != '\n');
		pos++;
		while (pos < strlen(text) && text[pos] != '\n') {
			pos++;
		}
		for (int i = 0; i < chars; i++) {
			pos++;
			if (text[pos] == '\n') break;
		}
	}
	
	if (pos > strlen(text)) pos = strlen(text);
	
	int y, x;
	getmaxyx(win, y, x);
	x++;
	if (cursorpos()[0] > y) mv_line(1);
}

void end() {
	while (pos < strlen(text) && text[pos] != '\n') {
		pos++;
	}
}

void begin() {
	while (pos > 0 && text[pos] != '\n') {
		pos--;
	}
	if (pos > 0) pos++;
}

int get_offset() {
	size_t off = 0;
	for (int i = 0; i < line_off; i++) {
		for (int i = off; i < strlen(text); i++) {
			if (text[i] == '\n') {
				off = i+1;
				break;
			}
		}
	}
	return off;
}

void mv_line(size_t count) {
	line_off += count;
}

void save() {
	file = fopen(filename, "wa");
	fprintf(file, "%s\n", text);
	fclose(file);
	
	char *str = (char *)malloc(255 * sizeof(char));
	sprintf(str, "Saved as %s", filename);
	message = str;
}

void handle_key(int key) {
	char ch = (char)key;

	if (isprint(key) || key == '\t') {
		add(ch);
		return;
	}
	
	switch(key) {
	case CTRL('s'):
		save();
		return;
	case CTRL('c'):
		stop = 1;
	case '\n':
		add('\n');
		break;
	case KEY_BACKSPACE:
		del();
		break;
	case KEY_LEFT:
		if (pos > 0) pos--;
		break;
	case KEY_RIGHT:
		if(pos < strlen(text)) pos++;
		break;
	case KEY_UP:
		up();
		break;
	case KEY_DOWN:
		down();
		break;
	case KEY_END:
		end();
		break;
	case KEY_HOME:
		begin();
		break;
	case KEY_DC:
		pos++;
		del();
		break;
	case KEY_NPAGE:
        for(int i = 0; i < 5; i++) {
    		down();
        }
		mv_line(5);
		break;
	case KEY_PPAGE:
        for (int i = 0; i < 5; i++) {
    		up();
        }
		mv_line(-5);
		break;
	default:
		break;
	}
}