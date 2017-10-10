.PHONY: clean

loser: MP1.c list_file.c list_file.h md5.c md5.h 
	gcc -std=c99 -Wall -O2 list_file.h list_file.c md5.h md5.c MP1.c -lm -o loser

clean: 
	rm -f loser
