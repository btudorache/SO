# Tema 3 sisteme de operare - Loader de executabile

334CA Tudorache Bogdan Mihai

Tema contine implementarea unui loader de executabile simplist
in limbajul **C** folosind API-ul **C POSIX**. 

# Implementare

Implementarea loader-ului contine cateva puncte cheie despre care
mertia vorbit mai in detaliu:

* Inregistrarea handler-ului de semnale **SIGSEV**

* Implementarea unei structuri cu care putem tine cont de status-ul maparii paginilor

* Maparea executabilului in memorie

* Tratarea propriu-zisa a page fault-urilor

## Inregistrarea handler-ului de semnale SIGSEV

Pentru a inregistra handler-ul de page fault-uri, am utilizat resursele clasice, adica
structura de tip ```struct sigaction``` si functia ```sigaction()```, in care am populat
si campul ```struct sigaction *oldact``` pentru a pastra o referinta la handler-ul default
de SIGSEV. Pentru a obtine mai multe informatii despre cum s-a generat semnalul (cum ar fi adresa 
la care s-a generat), am populat campul ```sigaction.sa_flags``` cu **SA_SIGINFO**.

## Implementarea unei structuri cu care putem tine cont de status-ul maparii paginilor

Aceasta structura este una foarte simplista, care se construieste
pentru fiecare segment de date in parte, si care contine:

* numarul de pagini virtuale din cadrul segmentului

* numarul de pagini fizice din cadrul segmentului

* un array de flag-uri de dimensiunea numarului de pagini virtuale
care specifica daca pagina a fost mapata sau nu

Structura se construieste pe baza structurii asociate unui segment ```so_seg_t```

## Maparea executabilului in memorie

Pe masura ce se ruleaza executabilul si se gasesc zone de memorie care
nu sunt mapate, pe langa necesitatea maparii lor, mai trebuie sa copiem
si datele din executabil in acea zona de memorie, asa ca a fost nevoie
de maparea executabilului in memorie folosind functia ```mmap()```

## Tratarea propriu-zisa a page fault-urilor

Pentru tratarea page fault-urilor, am respectat in mare parte indicatiile
specificate in enuntul temei:

* Daca se genereaza fault la o adresa care nu se gaseste in nici un
segment, inseamna ca a avut loc un acces invalid la memorie "pe bune",
si se apeleaza handler-ul default

* Daca se genereaza fault la o adresa care se gaseste intr-un segment dar
pagina este deja mapata, inseamna ca se incearca accesarea acelei zone cu
permisiuni pe care nu le avem, deci se apeleaza iar handler-ul default

* In schimb, daca fault-ul se genereaza la o adresa care se gaseste in
segmente si pagina nu este mapata: 

  * se marcheaza pagina respectiva ca fiind mapata

  * se mapeaza pagina cu permisiuni de write
    initial pentru a fi siguri ca putem copia datele din executabil
    folosind ```mmap()```

  * copiem datele din executabil in zona de memorie mapata in functie de offset-ul specificat in structura segmentului

  * re-mapam zona de memorie cu permisiunile specificate in segment cu ```mprotect()```
