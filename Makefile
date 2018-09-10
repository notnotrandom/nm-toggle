all : nm-toggle

nm-toggle : nm-toggle.c
	gcc -Wall -o nm-toggle nm-toggle.c

clean :
	rm nm-toggle

.PHONY : clean
