main: main.o canard.o funccanard.o bitlevel.o
	riscv64-unknown-elf-gcc -o main main.o canard.o funccanard.o bitlevel.o -lm -Wall

main.o: main.c 
	riscv64-unknown-elf-gcc -c main.c -Wall -lm -o main.o

canard.o:
	riscv64-unknown-elf-gcc -c libcanard/canard.c -Wall -o canard.o

funccanard.o: 
	riscv64-unknown-elf-gcc -c libcanard/funccanard.c -Wall -lm -o funccanard.o

bitlevel.o: 
	riscv64-unknown-elf-gcc -c libcanard/bitlevel.c -Wall -lm -o bitlevel.o

clean: 
	rm *.o main