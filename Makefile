LIBS=ncurses
CFLAGS=`pkg-config --cflags ${LIBS}`
LDFLAGS=`pkg-config --libs ${LIBS}`
OPTIONS=-O3 -Wall -Wextra -Werror

editor: editor.c
	gcc editor.c -o editor ${CFLAGS} ${LDFLAGS} ${OPTIONS}
