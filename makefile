#TODO

CC = gcc
CFLAGS = -Wall -Wextra -Werror -fPIC -O2

SRC = sound_seg.c
OBJ = sound_seg.o

all: $(OBJ)

$(OBJ): $(SRC) $(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ)
