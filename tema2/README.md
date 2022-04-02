# Tema 2 sisteme de operare - Biblioteca stdio

334CA Tudorache Bogdan Mihai

Tema contine implementarea unor functii din biblioteca **stdio.h** 
in limbajul **C** folosind API-ul **C POSIX**. 

# Implementare

Implementarea celor mai multe functii din cadrul acestei biblioteci
este de la sine inteleasa, insa exista cateva puncte care merita 
discutate mai in detaliu:

* forma structurii SO_FILE

* implementarea functiei ```so_fgetc```

* implementarea functiei ```so_fputc```

* implementarea functiei ```so_fseek```

* implementarea functiei ```so_popen```

## forma structurii SO_FILE

Ce este special la aceasta biblioteca fata de simpla utilizare
a unor operatii de *read/write* pe niste fisiere este ca foloseste
buffering. Spre exemplu, cand se apeleaza un simplu ```fgetc```, de
fapt un buffer intern va citi mai multe caractere din fisier in ideea 
ca probabil for fi folosite pe viitor, si evitam de asemenea apeluri 
de sistem si accesari inutile ale hard-ului care are o viteza mult mai
lenta ca memoria ram.

Structura SO_FILE contine un array *file_buffer* de dimensiune 4096 prin
intermediul caruia se face buffering atat pentru operatiile de read,
cat si pentru cele de write. 

In afara de aceste buffer, mai contine variate informatii utile despre 
contextul fisierului, cum ar fi: file descriptorul fisierului, un flag
de eroare, un flag de EOF, pozitia in fisier, pozitia in buffer si
un flag care specifica care a fost tipul ultimei operatii pe fisier.

## implementarea functiei ```so_fgetc```

implementarea metodei ```int so_fgetc(SO_FILE *stream)``` s-a facut
tinand cont de buffering-ul mentionat anterior. Daca bufferul este gol,
se face un apel de sistem pentru umplerea bufferului, iar pentru citiri viitoare,
se va intoare direct informatia din buffer.

Daca "cursorul" bufferului a ajuns la final, buffer-ul se reseteaza si se face
alt apel de sistem pentru a-l umple iar.

Functia ```so_fread``` foloseste intern apeluri repetate ale ```so_fgetc```.

## implementarea functiei ```so_fputc```

Implementarea aceste functii este "in oglinda" fata de ```so_fgetc```.
apelurile aceste functii scriu in buffer-ul intern pana cand acesta se umple.
Cand buffer-ul este plin, se face un apel de write, tot in ideea de a evita
apeluri de sistem care nu sunt necesare.

Functia ```so_fwrite``` foloseste intern apeluri repetate ale ```so_fputc```.

Daca un apel de sistem de write nu reuseste sa scrie tot buffer-ul, vor avea
loc apeluri repetate de write pana cand tot buffer-ul va fi scris cu 
succes in fisier.

## implementarea functiei ```so_fseek```

Aceasta functie este folosita atat pentru a modifica cursorul intern 
al fisierului deschis, dar si pentru a "reseta" buffer-ul intern 
intre operatii consecutive de *write -> read* sau *read -> write*.

Aceasta resetare este necesara pentru a asigura comportamentul corect 
al functionalitatii de buffering.

## implementarea functiei ```so_popen```

Functia ```so_popen``` este folosita pentru a citi de la stdout-ul,
sau de a scrie la stdin-ul unui proces nou specificat. Aceasta functionalitate
este implementata prin *forc/exec* pentru a porni un nou proces, si prin
 *pipe* pentru a creea o legatura comuna prin care pot comunica procesele. 
