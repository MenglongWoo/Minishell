CC=gcc
CFLAGS=-Wall -std=c99
LDFLAGS=-lreadline

SOURCES=shell.c
PROGRAMS=shell
OBJS=$(SOURCES:.c=.o)

all: $(PROGRAMS)

shell: 
	$(CC) $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $(PROGRAMS)

.PHONY: clean
clean:
	rm -rf *.o $(PROGRAMS)
