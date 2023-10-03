main: main.o canard.o funccanard.o bitlevel.o
	gcc -o main main.o canard.o funccanard.o bitlevel.o -lm -Wall

main.o: main.c 
	gcc -c main.c -Wall -lm -o main.o

canard.o:
	gcc -c libcanard/canard.c -Wall -o canard.o

funccanard.o: 
	gcc -c libcanard/funccanard.c -Wall -lm -o funccanard.o

bitlevel.o: 
	gcc -c libcanard/bitlevel.c -Wall -lm -o bitlevel.o

clean: 
	rm *.o main