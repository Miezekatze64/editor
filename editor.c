#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>


#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

#define SYNLEN 8

int pos = 0;
int selection_start = 0, selection_end = 0;
int line_off = 0;

volatile sig_atomic_t stop;
WINDOW* win;

FILE *file;
char *filename;
char *message;
bool hasfile = false;
bool save_as = false;

char *text;
char **argv;
char **syntax[SYNLEN];
int syntax_size[SYNLEN] = {0, 0, 0, 0, 0, 0, 0, 0};

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
void show(char *string, int color, int position);
void showc(char c, int color, int position);

/* string functions */

char *copy(char *from) {
	char *to = malloc(strlen(from));
	memcpy(to, from, strlen(from));
	to[strlen(from)] = '\0';
	return to;
}

/* main funtion */

int main(int argc, char **argv_in) {
	argv = argv_in;
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
	
	
	init_pair(10, COLOR_CYAN, COLOR_BLACK);		//syntax group 0
	init_pair(11, COLOR_GREEN, COLOR_BLACK);	//syntax group 1
	init_pair(12, COLOR_WHITE, COLOR_BLACK);	//syntax group 2
	init_pair(13, COLOR_RED, COLOR_BLACK);		//syntax group 3
	init_pair(14, COLOR_BLUE, COLOR_BLACK);		//syntax group 4
	init_pair(15, COLOR_YELLOW, COLOR_BLACK);	//syntax group 5
	init_pair(16, COLOR_MAGENTA, COLOR_BLACK);	//syntax group 6
	init_pair(17, COLOR_WHITE, COLOR_YELLOW);	//syntax group 7
	
	init_pair(20, COLOR_BLACK, COLOR_WHITE);	//selection
	
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
		move(y-1, 0);
		clrtoeol();
		if (message != NULL) {
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
	
	if (strchr(filename, '.') == NULL) {
		lang = copy(filename);
	} else for (int i = strlen(filename); i > 0; i--) {
		if (filename[i] == '.') {
			lang = copy(filename+i+1);
			break;
		}
	}

	if (strlen(lang) == 0) return;
	
	char *langfile = malloc(strlen(lang)+7+8);
	char *path = malloc(strlen(argv[0]));
	
	int index = 0;
	for (int i = strlen(argv[0])-1; i >= 0; i--) {
		if (argv[0][i] == '/') {
			index = i;
			break;
		}
	}
	
	memcpy(path, argv[0], index+1);
	path[index+1] = '\0';
	
	
	langfile[0] = '\0';
	strcat(langfile, path);
	strcat(langfile, ".syntax/");
	strcat(langfile, lang);
	strcat(langfile, ".syntax");


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
	for (int group = 0; group < SYNLEN; group++) {
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
				syntax[group][synindex] = copy(working);
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
	char *str = getText();
	char *working = malloc(255);
	int index = 0;
	int lines = 0;
	
	int maxlines, maxX;
	getmaxyx(win, maxlines, maxX);
	
	int comment = 0;
	char *comment_suf = malloc(1);
	int xpos = 0;
	for (int i = 0; i < strlen(str); i++) {
		xpos++;
		if (!(str[i] == ' ' || str[i] == '\n' || (str[i] == '(' || str[i] == ')' || str[i] == '[' || str[i] == ']' || str[i] == '{' || str[i] == '}' || str[i] == '\t' || str[i] == ';' || str[i] == ':' || str[i] == ',' || str[i] == '='  || str[i] == '.' || i >= strlen(str)-1))) {
			working[index] = str[i];
			index++;
		} else {
			working[index] = '\0';
			
			int y, x;
			getyx(win, y, x);
			x++;
			if (y >= maxlines-1) return;
			if (str[i] == '\n') {
				lines++;
				xpos = 0;
			}
			
			if (xpos > maxX) {
				xpos -= maxX+1;
				lines++;
			}
	
			char *to_show = malloc(strlen(working)+4);
			memcpy(to_show, working, strlen(working));
			to_show[index] = str[i];
			to_show[index+1] = '\0';
			index = 0;
			
			bool found = false;
			int highlight = 0;
			
			for (int group = 0; group < SYNLEN; group++) {
				highlight = 0;
				for (int j = 0; j < syntax_size[group]; j++) {
					char *compare = syntax[group][j];
					
					if (strchr(compare, '?') != NULL) {
						char *pre = malloc(strlen(compare));
						char *suf = malloc(strlen(compare));
						
						int position = (int)(strchr(compare, '?')-compare);
						for (int ctpos = 0; ctpos < strlen(compare)+1; ctpos++) {
							if (ctpos < position) {
								pre[ctpos] = compare[ctpos];
							} else if (ctpos == position) {
								pre[ctpos] = '\0';
							} else if (ctpos <= strlen(compare)) {
								if (position == strlen(compare)-1) {
									suf[ctpos-position-1] = '\n';
									suf[ctpos-position] = '\0';
								} else {
									suf[ctpos-position-1] = compare[ctpos];
								}
							} else {
								suf[ctpos-position-1] = '\0';
							}
						}
						
						if (strncmp(working, pre, strlen(pre)) == 0) {
							comment = 1;
							comment_suf = copy(suf);
						}
						if(comment == 1) {
							highlight = 1;
						}
						
						if (comment_suf)
						if (comment == 1 && strncmp(to_show+strlen(to_show)-strlen(comment_suf), comment_suf, strlen(comment_suf)) == 0) {
							highlight = 1;
							comment = 0;
						}
						
						if (comment_suf)
						if (comment == 1 && strncmp(working+strlen(working)-strlen(comment_suf), comment_suf, strlen(comment_suf)) == 0) {
							highlight = 1;
							comment = 0;
						}
						
						free(pre);
						free(suf);
						
					} else if (!comment || group == 6) {
						if (strcmp(compare, "*num*") == 0) {
							if (strspn(working, "-0123456789Lfd") == strlen(working)) {
								highlight = 1;
							} else {
								highlight = 0;
							}
						} else if (strcmp(compare, "*string*") == 0) {
							if (working[0] == '\"' && working[strlen(working)-1] == '\"') {
								highlight = 1;
							} else 	if (working[0] == '\'' && working[strlen(working)-1] == '\'') {
								highlight = 1;
							} else {
								highlight = 0;
							}
						} else if (strcmp(compare, working) == 0) {
							highlight = 1;
						} else {
							highlight = 0;
						}
					}
					
					
					if (highlight == 1) {
						show(working, 10+group, i);
						if (!comment) {
							showc(str[i], 0, i);
						} else {
							showc(str[i], 10+group, i);
						}
						found = true;
						break;
					}
				}
				if (found) break;
			}
			if (!found) show(to_show, 0, i);
			free(to_show);
			working = malloc(255);
		}			
	}
	free(str);	
	move(maxlines-1, 0);
	clrtoeol();
}

void show(char *string, int color, int position) {
	for (int i = 0; i < strlen(string); i++) {
		showc(string[i], color, position-strlen(string)+i);
	}
}

void showc(char c, int color, int position) {
/*	int first, second;
	if (selection_start < selection_end) {
		first = selection_start;
		second = selection_end;
	} else {
		first = selection_end;
		second = selection_start;		
	}*/
//	endwin();
//	printf("first: %d, second: %d", first, second);
//	exit(1);
/*	if ((first <= position) && (position < second)) {
		attron(COLOR_PAIR(20));
	} else {
*/		if (color != 0) attron(COLOR_PAIR(color));
		if (color != 0) attron(A_BOLD);
//	}
	printw("%c", c);
	attroff(COLOR_PAIR(color));
	attroff(A_BOLD);
}

char *getText() {
	size_t off = get_offset();
	char *t = malloc(strlen(text)-off+1);
	memcpy(t, text+off, strlen(text)-off+1);
	
	return t;
}

void add(char c) {
	char *new_array = malloc(strlen(text)+10 * sizeof(char));
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
	int maxX, maxY;
	getmaxyx(win, maxY, maxX);
	maxY++;
	int i;
	for (i = get_offset(); i < pos; i++) {
		if (text[i] == '\n' || x >= maxX-1) {
			y++;
			x = 0;
		} else {
			if (text[i] == '\t') {
				x += 4;
				x = (int)(x/4);
				x *= 4;
		    } else
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
	if (cursorpos()[0] >= y) mv_line(1);
}

void end() {
	while (pos < strlen(text) && text[pos] != '\n') {
		pos++;
	}
}

void begin() {
	if (pos > 0) if (text[pos] == '\n') pos--;
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

void nexline() {
	int tabs = 0;
	bool first_char = true;
	for (int i = pos-1; i > 0; i--) {
		if (first_char && text[i] == '{') tabs++;
		if (text[i] != ' ' && text[i] != '\t' && text[i] != '\n') first_char = false;
        if (text[i] == '\n') {
            i++;
            for (int j = i; j < pos; j++) {
                if (text[j] != '\t') {
                    tabs += j-i;
					break;
				}
			}
			break;
		}
	}
	add('\n');
	for (int i = 0; i < tabs; i++)
		add('\t');
}

void select_right() {
	selection_end++;
}

void select_left() {
	selection_end--;
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
		break;
	case '\n':
		nexline();
		selection_start = pos;
		break;
	case KEY_BACKSPACE:
		del();
		selection_start = pos;
		break;
	case KEY_LEFT:
		if (pos > 0) pos--;
		selection_start = pos;
		break;
	case KEY_RIGHT:
		if(pos < strlen(text)) pos++;
		selection_start = pos;
		break;
	case KEY_SRIGHT:
		select_right();
		break;
	case KEY_SLEFT:
		select_left();
		break;
	case KEY_UP:
		up();
		selection_start = pos;
		break;
	case KEY_DOWN:
		down();
		selection_start = pos;
		break;
	case KEY_END:
		end();
		break;
	case KEY_HOME:
		begin();
		selection_start = pos;
		break;
	case KEY_DC:
		if (pos < strlen(text)-1) {
			pos++;
			del();
		}
		selection_start = pos;
		break;
	case KEY_NPAGE:
        for(int i = 0; i < 10; i++) {
    		if (pos < strlen(text)-10) {
				down();
				mv_line(1);
			}
        }
		selection_start = pos;
		break;
	case KEY_PPAGE:
        for (int i = 0; i < 10; i++) {
			if (pos > 10) {
				up();
				mv_line(-1);
			}
        }
		selection_start = pos;
		break;
	default:
		break;
	}
}