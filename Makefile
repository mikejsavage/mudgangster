CC = gcc
SRCDIR = src
CFLAGS = -O2 -std=gnu99 $(shell pkg-config --cflags lua5.1)
LIBS = -lX11 -L/usr/X11R6/lib $(shell pkg-config --libs lua5.1)
WARNINGS = -Wall -Wextra -Werror
OBJS = main.o textbox.o input.o ui.o script.o
.PHONY: all

all: ${OBJS}
	${CC} -o mudGangster ${OBJS} ${CFLAGS} ${LIBS} ${WARNINGS}

%.o: ${SRCDIR}/%.c
	${CC} -c ${SRCDIR}/$*.c ${CFLAGS} ${LIBS} ${WARNINGS}
