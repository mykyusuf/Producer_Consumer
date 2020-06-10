all: myk

myk:
	gcc -c pCp.c
	gcc -o exe pCp.o

clean:
	rm *.o
