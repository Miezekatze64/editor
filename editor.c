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
bool save_as = false;

char *text;
char **syntax[5];
int syntax_size[5] = {0, 0, 0, 0, 0};

void add(char c);
void setcursor();
void del();
void handle_key(int key);
char *getText();
void setText();
void escape();
int get_offset();
void mv_line(size_t count);
void setempty();
int *cursorpos();
void loadsyntax();

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
				text[fsize-1] = '\0';
	
				hasfile = true;
			}
		} else {
			printf("Argument %s not found!", argv[0]);
			exit(1);
		}
	}
	
	loadsyntax();
	
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
    init_pair(1, COLOR_WHITE, COLOR_BLACK);		//standard text
    init_pair(2, COLOR_GREEN, COLOR_BLACK);		//messages at screen bottom
    init_pair(3, COLOR_RED, COLOR_BLACK);		//end of file
    
    
    init_pair(10, COLOR_YELLOW, COLOR_BLACK);	//syntax group 0
    init_pair(11, COLOR_WHITE, COLOR_BLACK);	//syntax group 1
    init_pair(12, COLOR_RED, COLOR_BLACK);	//syntax group 2
    init_pair(13, COLOR_BLUE, COLOR_BLACK);	//syntax group 3
    init_pair(14, COLOR_GREEN, COLOR_BLACK);	//syntax group 4
	
	while(!stop) {
		erase();
		attron(COLOR_PAIR(3));
		attron(A_BOLD);
		setempty();
		attron(COLOR_PAIR(1));
		attroff(A_BOLD);
		
		setText();
		
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

void loadsyntax() {
	if (filename == NULL) {
		return;
	}
	
	
	char *lang = malloc(1);
	lang[0] = '\0';
	for (int i = strlen(filename); i > 0; i--) {
		if (filename[i] == '.') {
			lang = malloc(strlen(filename)-i);
			memcpy(lang, filename+i+1, strlen(filename)-i);
			lang[strlen(filename)-i] = '\0';
			break;
		}
	}

	if (strlen(lang) == 0) return;
	
	char *langfile = malloc(strlen(lang)+7);
	memcpy(langfile, lang, strlen(lang));
	memcpy(langfile+strlen(lang), ".syntax", 7);
	
	langfile[strlen(lang)+7] = '\0';

	char *input = malloc(2);
	
	if(access(langfile , F_OK ) == 0) {
		FILE *file2;
		file2 = fopen(langfile, "rwb");
		fseek(file2, 0, SEEK_END);
		long fsize = ftell(file2);
		fseek(file2, 0, SEEK_SET);
		
		input = malloc(fsize + 1);
		fread(input, fsize, 1, file2);
		fclose(file2);
		input[fsize] = '\0';
	} else {
		perror("File error");
	}
		
	free(lang);
	free(langfile);
	
	char *working = malloc(1);
	
	int i = 0;
	for (int group = 0; group < 5; group++) {
		int index = 0;
		int synindex = 0;
		if (syntax[group] == NULL) {
			syntax[group] = malloc(strlen(input));
		}
		
		while (i < strlen(input)) {
			if (input[i] == '\n') {
				i++;
				break;
			}
			if (input[i] != '|') {
				working[index] = input[i];
				index++;
			} else {
				working[index] = '\0';
				index = 0;
				syntax[group][synindex] = (char *)malloc(10);
				memcpy(syntax[group][synindex], working, strlen(working)+1);
				working = malloc(20);
				syntax_size[group]++;
				synindex++;
			}
			i++;
		}
	}
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

void setText() {
	if (syntax[0] != NULL) {
		char *str = getText();
		char *working = malloc(100);
		int index = 0;
		int lines = 0;
		
		int maxlines, x;
		getmaxyx(win, maxlines, x);
		x++;
		
		for (int i = 0; i < strlen(str); i++) {
			if (str[i] != ' ' && str[i] != '\n' && str[i] != '(' && str[i] != ')' && str[i] != '[' && str[i] != ']' && str[i] != '{' && str[i] != '}' && str[i] != '*' &&  str[i] != '\t' &&  str[i] != ';' &&  str[i] != ':' && i < strlen(str)-1) {
				working[index] = str[i];
				index++;
			} else {
				working[index] = '\0';
				
				if (lines == maxlines) break;
				if (str[i] == '\n') lines++;
				
				char *to_show = malloc(strlen(working)+4);
				memcpy(to_show, working, strlen(working));
				to_show[index] = str[i];
				to_show[index+1] = '\0';
				index = 0;
				
				bool found = false;
				for (int group = 0; group < 5; group++) {
					for (int j = 0; j < syntax_size[group]; j++) {
						char *compare = syntax[group][j];
						if (strcmp(compare, working) == 0) {
							attron(COLOR_PAIR(10+group));
							attron(A_BOLD);
							printw("%s", working);
							attron(COLOR_PAIR(1));
							attroff(A_BOLD);
							printw("%c", str[i]);
							found = true;
							break;
						}
					}
					if (found) break;
				}
				if (!found) printw("%s", to_show);
				free(to_show);
				working = malloc(100);
			}			
		}
		free(str);
	} else {
		printw("%s", text);
	}
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
	if (filename == NULL) {
		save_as = true;
		handle_key(0);
		return;
	}
	file = fopen(filename, "wa");
	fprintf(file, "%s\n", text);
	fclose(file);
	
	char *str = (char *)malloc(255 * sizeof(char));
	sprintf(str, "Saved as %s!", filename);
	message = str;
}

void handle_key(int key) {
	char ch = (char)key;
	
	if (save_as) {
		if ( ch == '\n' ) {
			save_as = false;
		    save();
		    return;
		} else if (ch == CTRL('c')) {
			save_as = false;
			message = NULL;
			return;
		} else if (ch == KEY_BACKSPACE || ch == CTRL('g')) {
			if (filename != NULL) {
				int length;
				if (message == NULL) {
					message = malloc(1);
				}
				
				length = strlen(filename);
			
				char *str = malloc(length);
				memcpy(str, filename, strlen(filename));
				str[length-1] = '\0';
				
				memcpy(filename, str, length);
				memcpy(message, "Enter filename: ", 16);
				memcpy(message+16, filename, strlen(filename)+1);
			}
			return;
		} else if (isprint(key) || key == 0) {
		int length;
		if (message == NULL) {
			length = 2;
			message = malloc(18);
		} else {
			length = strlen(message)+2;
		}
		
			length *= sizeof(char);
			
			char *str = malloc(length);
			
			str[length-1] = '\0';
			str[length-2] = ch;
			
			if (filename == NULL) {
				filename = malloc(2);
				memcpy(filename, str, length);
			} else {
				memcpy(filename+strlen(filename), str, length);
			}
			memcpy(message, "Enter filename: ", 16);
			memcpy(message+16, filename, strlen(filename)+1);
		}
	    return;
	}

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