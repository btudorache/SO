# Tema 1 sisteme de operare - Preprocesor de fisiere C

334CA Tudorache Bogdan Mihai

Tema contine implementarea unui **preprocesor de fisiere C** in limbajul **C**. 
Aceasta etapa de preprocesare implica: 

* Expandarea directivelor de tipul *#include* (in mod recursiv) 

* Expandarea directivelor de tipul *#define* si *#undef*  

* Expandarea directivelor de includere conditionala a codului: *#if*, *#elif*, *#else*, *#endif* etc...

# Implementare

Implementarea contine cateva etape mari care pot fi discutate mai pe larg:

1. Definirea/implementarea unui **hashmap** care functioneaza cu string-uri

2. Expandarea directivelor *#include* de adaugare de noi definitii in fisierul principal

3. Expandarea celorlalte directive: *#define*, *#if*, *#else*, *#undef* etc...

## Implementarea unui hashmap

Am implementat un hashmap rudimentar care poate avea chei si valori de tip 
char[], si o politica de tratare a coliziunilor prin linear probing. 
Am considerat ca nu are rost o implementare mai generica de hashmap pentru
acest procesor, avand in vedere ca dorim doar o mapare de la char[] la char[].

Hashmap-ul expune metodele: ```has_key```, ```get_value```, ```put_value```. 
Functia de hashing pentru string-uri este luata de la [aceasta sursa](http://www.cse.yorku.ca/~oz/hash.html)

## Expandarea directivelor *#include*

Pentru expandarea acestor directive, flow-ul este urmatorul: verificam
existenta fisierului header in folderul curent, iar daca nu e gasit, incercam
pe rand in folderul extras din calea sursei date ca input si apoi in folderele
specificate la input prin flagul -I. Daca nu e gasit un astfel de fisier,
terminam programul cu un cod de eroare.

In cazul in care s-a gasit fisierul specificat in *#include*, il deschidem si
extragem liniile fisierului. Daca gasim inca o directiva *#include* in header,
vom construi recursiv toata structura. La final, vom "insera" liniile extrase 
de la header in fisierul principal.

## Expandarea celorlalte directive

Expandarea celorlalte directive se face secvential, linie cu linie.
In aceasta categorie avem inca 2 mari subcategorii de directive:

1. directive de tip *#define*

2. directive conditiionale

### Directive *#define*

Exista suport pentru mai multe tipuri de define-uri (multi-line, imbricate, simple). In esenta, inlocuirea define-uri s-a facut prin adaugarea lor intr-un
hashmap si translatarea in liniile in care sunt prezente folosind ```strtok```
si **aritmetica cu pointeri**.

### Directive conditionale

La directivele conditionale, se hotaraste adaugarea sau nu a unei linii pe baza
directivelor conditionale care, atunci cand sunt intalnite, declanseaza
modificarea a doua variabile numite: ```include_line``` si ```chosen_branch```
pe baza carora se decide daca ar trebui sa includem linia sau nu.

## Alte detalii

Imi functioneaza toate testele daca le rulez pe fiecare in parte. 

Pe Linux, imi merg toate testele, mai putin ultimul (care imi da rezultatul
 corect local), dar n-am reusit sa detectez problema.

Pe Windows imi apar niste erori de memcheck care in mod normal nu imi apar pe 
linux, si de asemenea nu functioneaza ultimul test.

Tin sa mentionez ca am verificat fiecare alocare sau operatie cu fisiere si nu
am reusit sa detectez problemele.


