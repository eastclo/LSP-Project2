all : ssu_mntr daemon

ssu_mntr : main.o ssu_mntr.o
	gcc main.o ssu_mntr.o -o ssu_mntr

daemon : daemon.o
	gcc daemon.o -o daemon

main.o : main.c ssu_mntr.h
	gcc -c main.c

ssu_mntr.o : ssu_mntr.c ssu_mntr.h
	gcc -c ssu_mntr.c

daemon.o : daemon.c daemon.h
	gcc -c daemon.c

clean :
	rm *.o
	rm ssu_mntr
	rm daemon
