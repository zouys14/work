out: test.o ssd_group.o
	gcc -o out test.o ssd_group.o

test.o: ssd_group.h test.c
	gcc test.c -fopenmp -llightnvm  -o test.o 
ssd_group.o: ssd_group.h ssd_group.c
	gcc ssd_group.c -fopenmp -llightnvm -o ssd_group.o 

clean:
	rm test.o ssd_group.o out
