CC = gcc
CFLAGS = -g
files = $(wildcard *.c)

main: $(files)

clean:
	rm -f *.o
	rm main
