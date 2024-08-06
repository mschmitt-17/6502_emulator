CC = gcc
CFLAGS = -g
files = $(wildcard *.c) $(wildcard */*.c)

main: $(files)

clean:
	rm -f *.o
	rm main
