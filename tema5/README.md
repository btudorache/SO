# Tema 5 sisteme de operare - Server web asincron
334CA Tudorache Bogdan Mihai

Tema contine implementarea unui server web asincron de fisiere 
in limbajul **C** folosind API-ul **C POSIX**. 

# Implementare

In cadrul temei am implementat doar transmiterea fisierelor statice.

## Transmiterea fisierelor statice

Initial, cand un nou client se conecteaza si primim o cerere http, este parsata cererea si se detecteaza
daca se cere un fisier static. In caz afirmativ, initial se incearca trimiterea
fisierului in intregime folosind ```sendfile()```. Tinand cont ca socketii de retea
sunt in modul non-blocant, acest lucru se face printr-o structura do while.

Daca fisierul nu poate fi transmis in intregime (poate buffer-ul de retea de transmitere
este plin), se activeaza evenimentele de tip EPOLLOUT pe socket-ul respectiv, iar cand acest
eveniment este declansat, incercam sa trimitem in continuare restul fisierului. In aceasta faza,
daca fisierul a fost trimis in intregime, se finalizeaza operatia prin inchiderea conexiunii.

Este tratata si situatia in care calea detectata prin parsarea request-ului nu cere un fisier
static sau dinamic.