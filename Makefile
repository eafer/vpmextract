CC = gcc
CFLAGS = --std=gnu99 -O3 -Wall -Wextra -fno-strict-aliasing

vpmextract: vpmextract.c
	$(CC) $(CPPFLAGS) $(CFLAGS) vpmextract.c -o vpmextract
clean:
	rm -f vpmextract
