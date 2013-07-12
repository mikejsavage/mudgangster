CC = gcc
SRCDIR = src
CFLAGS = -O2 -std=gnu99
LIBS = -lX11 -L/usr/X11R6/lib -llua
WARNINGS = -Wall -Wextra -Werror
OBJS = main.o textbox.o input.o ui.o script.o
.PHONY: all

all: ${OBJS}
	${CC} -o mudGangster ${OBJS} ${CFLAGS} ${LIBS} ${WARNINGS}

%.o: ${SRCDIR}/%.c
	${CC} -c ${SRCDIR}/$*.c ${CFLAGS} ${LIBS} ${WARNINGS}
