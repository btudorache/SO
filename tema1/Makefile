CFLAGS=/nologo /W3 /MD

build: so-cpp.exe

so-cpp.exe: so-cpp.obj hashmap.obj
	link hashmap.obj so-cpp.obj /Feso-cpp.exe

so-cpp.obj: so-cpp.c

hashmap.obj: hashmap.c

clean:
	del *.obj *.exe
