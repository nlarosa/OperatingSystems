
all: mandel mandelmovie

mandel: mandel.o bitmap.o
	gcc mandel.o bitmap.o -o mandel -lpthread

mandel_old: mandel_old.o bitmap.o
	gcc mandel_old.o bitmap.o -o mandel_old -lpthread

mandel_old.o: mandel_old.c
	gcc -Wall -g -c mandel_old.c -o mandel_old.c -pthread

mandel.o: mandel.c
	gcc -Wall -g -c mandel.c -o mandel.o -pthread

bitmap.o: bitmap.c
	gcc -Wall -g -c bitmap.c -o bitmap.o

mandelmovie: mandelmovie.c
	gcc mandelmovie.c -o mandelmovie -Wall

clean:
	rm -f mandel.o bitmap.o mandel
