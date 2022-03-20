CFLAGS=/nologo /W4 /EHsc /Za

all: so-cpp

so-cpp: so-cpp.o hashmap.o
	cl hashmap.obj so-cpp.obj /Feso-cpp.exe

so-cpp.o: so-cpp.c
	cl /c so-cpp.c

hashmap.o: hashmap.c
	cl /c hashmap.c

clean:
	del *.obj *.exe
