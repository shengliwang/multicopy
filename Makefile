all:main.o multicopy.o
	gcc -g -pthread main.o multicopy.o -o app

main.o: main.c
	gcc -c main.c -o main.o

multicopy.o: multicopy.c
	gcc -c multicopy.c -o multicopy.o

clean:
	rm -f *.o app
