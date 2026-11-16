CC = gcc
CFLAGS = -Wall -Wextra -O2

banker: banker.c
	$(CC) $(CFLAGS) -o banker banker.c

clean:
	rm -f banker result.txt

.PHONY: clean