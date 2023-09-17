main: main.o canard.o funccanard.o
	gcc main.o canard.o funccanard.o -lm -Wall -o main

main.o: main.c 
	gcc -c main.c -Wall -lm -o main.o

canard.o:
	gcc -c libcanard/canard.c -Wall -o canard.o

funccanard.o: 
	gcc -c libcanard/funccanard.c -Wall -lm -o funccanard.o

clean: 
	rm *.o main