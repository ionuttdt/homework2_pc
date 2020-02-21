Duta Viorel-Ionut
321CB
Tema 2 PC
Tema respecta toate conditiile din enunt.

Observatii:
	Pentru a intelege mai usor modul in care am implementat tema va rog sa cititi
prima data comentariile din helpers.h.


Resurse:
	Pentru implementarea acestei teme am folost scheletul laboratoarelor 6 si 7.


Descriere server:
	Serverul incepe prin deschiderea a doi socketi (TCP, respectiv UDP) pe care
o sa primeasca mesaje de la clienti. Serverul va accepta fiecare client TCP
nou conectat.
	Cand se conecteaza un nou client TCP, va fi afisat un mesaj specific acestei
actiuni, dar doar daca este prima oara cand se conecteaza. Pentru a stoca
informatiile despre fiecare client conectat am construit o structura de date
care foloseste liste simplu inlantuite. Pentru fiecare client memorez ID-ul, 
portul, socket, un int care imi spune daca clientul este online (0 = deconectat,
1 = conectat) o lista cu topicurile la care acesta este abonat. Daca un client da
comanda exit, serverul ii schimba starea din 1 in 0.
	Daca serverul primeste mesaj de la un client UDP, face parsarea si dupa trimite
mesajul construit in bufferaux care toti clientii conectati la topicul specificat
in mesaj (folosind functia send_tcp(AList clienti, char *topic, char *buffer)).
Mai multe detalii despre modul in care se face parasarea sunt specificate in
codul sursa.
	Serverul poate primi mesaje si de la tastatura. Pentru comanda exit acesta
inchide conexiunea cu toti clientii.
	Clientii TCP pot trimite serverului 3 tipuri de mesaje (exit, subscribe si
unsubscribe). Pentru comenada exit sechimba doar starea clientului din conectat
in deconectat, iar pentru subscribe si unsubscribe se adauga un nou topic in
lista de topicuri, respectiv se elimina. Aceste operatii se fac folosind functiile
cu acelasi nume din helpers.h (descrise mai in amanunt acolo).


Descriere subscriber:
	Clientul pastreaza aceeasi functionalitate ca si cel din laboratorul TCP, 
Pentru comanda exit se trimite serverului un mesaj gol si acesta o sa il 
deconecteze, iar restul mesajelor (primite de la tastatura) sunt trimise direct
serverului care o sa le faca parsarea si verifica daca este vorba de comanda
subscribe sau unsubscribe.
