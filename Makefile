SRCDIR = src
CXXFLAGS = -std=c++11 $(shell pkg-config --cflags lua51) -O0 -ggdb
LIBS = -lm -lX11 -L/usr/X11R6/lib $(shell pkg-config --libs lua51)
WARNINGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wshadow -Wcast-align -Wstrict-overflow -Wvla
OBJS = main.o textbox.o input.o ui.o script.o
.PHONY: all

all: ${OBJS}
	${CXX} -o mudGangster ${OBJS} ${CXXFLAGS} ${LIBS} ${WARNINGS}

%.o: ${SRCDIR}/%.cc
	${CXX} -c ${SRCDIR}/$*.cc ${CXXFLAGS} ${LIBS} ${WARNINGS}

clean:
	rm -f mudGangster ${OBJS}
