#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>

int pos = 0;
volatile sig_atomic_t stop;
WINDOW* win;

FILE* logfile;
FILE* file;
bool hasfile = false;

char *text;

void add(char c);
void setcursor();
void del();
void handle_key(int key);


int main(int argc, char **argv) {
	if (argc > 1) {
		if (argv[1][0] != '-') {
			//parse as file
			char* str = argv[1];
			file = fopen(str, "rb");

			fseek(file, 0, SEEK_END);
			long fsize = ftell(file);
			fseek(file, 0, SEEK_SET);  /* same as rewind(f); */
			
			text = malloc(fsize + 1);
			printf("Size: %d\n", (int)fsize);

			fread(text, fsize, 1, file);
			fclose(file);
			
			text[fsize] = '\0';

			for (int i = 0; i < strlen(text); i++) {
				if (text[i] < 4) {
					text[i] = '?';
				} else if(text[i] == '\t') {
//					text[i] = ' ';
					//TODO: replace tabs with 4 spaces instead of 1
					/*
					char *new_array = malloc(strlen(text)+5 * sizeof(char));
					new_array[strlen(text)+5] = '\0';

					for (int j = 0; j < strlen(new_array)+4; j++) {
						if (j < i) {
							new_array[j] = text[j];
						} else if (j > i+6) {
							new_array[j] = text[j-4];
						} else {
							new_array[j] = ' ';
						}
					}

					text = new_array;*/
				}
			}

			hasfile = true;
		} else {
			printf("Argument %s not found", argv[0]);
		}
	}
	
	logfile = fopen("file.log", "w");

    if (!hasfile) {
		text = malloc(1);
		text[0] = '\0';
	}

	if (logfile == NULL) {
		printf("Error opening file!\n");
		exit(1);
	}
	
	win = initscr();
	noecho();
	cbreak();
	keypad(win, true);
	
	while(!stop) {
		clear();
		printw(text, 0, 0);
		
		setcursor();
		handle_key(getch());
		refresh();
	}
	endwin();
	
	fclose(logfile);
	return 0;
}

void add(char c) {
	char *new_array = malloc(strlen(text)+2 * sizeof(char));
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

void setcursor() {
	int x = 0;
	int y = 0;
	int i;
	for (i = 0; i < pos; i++) {
		if (text[i] == '\n') {
			y++;
			x = 0;
		} else {
			x++;
		}
	}
	move(y, x);
	fflush(logfile);
}

void del() {
	if (pos <= 0) return;
	char *new_array = malloc((strlen(text)-1) * sizeof(char));

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
	while (pos > 0 && text[pos] != '\n') {
		pos--;
		chars++;
	}
	pos--;
	while (pos > 0 && text[pos] != '\n') {
		pos--;
	}
	if (pos == 0) chars--;
	pos += chars;
}

void down() {
	int chars = 0;
	while (pos >= 0 && text[pos] != '\n') {
		pos--;
		chars++;
	}
	pos++;
	while (pos < strlen(text) && text[pos] != '\n') {
		pos++;
	}

	if (pos == strlen(text)-1) chars++;

	pos += chars;
	if (pos > strlen(text)) pos -= chars+1;
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

void handle_key(int key) {
	char ch = (char)key;
	
	if (isprint(key)) {
		add(ch);
	} else if(key == '\n') {
		add('\n');
	} else if(key == KEY_BACKSPACE) {
		del();
	} else if(key == KEY_LEFT) {
		if (pos > 0) pos--;
	} else if(key == KEY_RIGHT) {
		if(pos < strlen(text)) pos++;
	} else if (key == KEY_UP) {
		up();
	} else if (key == KEY_DOWN) {
		down();
	} else if (key == KEY_END) {
		end();
	} else if (key == KEY_HOME) {
		begin();
	} else {
	
	}
}
