all: so-cpp

so-cpp: so-cpp.o hashmap.o
	gcc hashmap.o so-cpp.o -o so-cpp

so-cpp.o: so-cpp.c
	gcc -Wall -c so-cpp.c

hashmap.o: hashmap.c
	gcc -Wall -c hashmap.c

clean:
	rm -f *.o so-cpp
