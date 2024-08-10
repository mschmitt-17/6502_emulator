CC = gcc
CFLAGS = -g -lglfw -ldl -I/usr/include/freetype2 -lfreetype
cfiles = $(wildcard *.c) $(wildcard */*.c)

all:
	$(CC) $(cfiles) -o main $(CFLAGS)

clean:
	rm -f *.o
	rm main
