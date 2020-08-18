PROG_NAME=hexo

CC=gcc
CFLAGS=-I. 
LIBS=-lncurses -llua -lm -ldl
DEPS = globals.h hex.h plugsel.h console.h
OBJ = main.o globals.o hex.o plugsel.o console.o

RM= rm -f

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all:	$(PROG_NAME)

$(PROG_NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	$(RM) $(OBJ) $(PROG_NAME)
