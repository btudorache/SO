build: libscheduler

libscheduler: so_scheduler.o
	gcc -shared so_scheduler.o -o libscheduler.so

so_scheduler.o: so_scheduler.c
	gcc -Wall -fPIC -c so_scheduler.c -lpthread

clean:
	rm -f *.o libscheduler.so
