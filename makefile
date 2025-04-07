#TODO

CC = gcc
CFLAGS = -Wall -Wextra -Werror -fPIC

sound_seg.o: sound_seg.c
	$(CC) $(CFLAGS) -c sound_seg.c -o sound_seg.o

clean:
	rm -f sound_seg.o

