CFLAGS=/nologo /W4 /EHsc /Za /MD

all: so-cpp.exe

so-cpp.exe: so-cpp.obj hashmap.obj
	cl hashmap.obj so-cpp.obj /Feso-cpp.exe

so-cpp.obj: so-cpp.c
	cl /c so-cpp.c

hashmap.obj: hashmap.c
	cl /c hashmap.c

clean:
	del *.obj *.exe
