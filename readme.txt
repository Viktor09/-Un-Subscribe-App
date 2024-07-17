Server.c

Pentru inceput am facut o structura care contine idul clinetului, socketul, topicurile care
sunt variabile, un counter pt ele si un flag pentru a vedea activitatea clientului.

Apoi setam ipurile si porturile pentru udp si tcp. Dupa care le adaugam impreuna cu file descriptorul
de input in in FD_SET pentru a le putea accesa prin intermediul multiplexarii I/O.

Acum am trat niste cazuri, cel de input in care e valabila doar comanda exit, cel de client tcp
in care accept conexiunea si o sa primiesc id-ul, port-ul si ip-ul lui. Dupa o sa verificam daca clientul
e deja conectat si daca e atunci o sa il inchidem, daca clientul a fost deconectat atunci o sa ne folosim
de flagul din structura de mai sus si o sa il adaug la loc fara sa folosesc mai multa memorie. Si in cazul
in care nu exista il adaugam in structura dupa campuri.
In cazul cu clientul udp o sa primesc pachetul cu datele care urmeaza sa fie prelucrate.
Aici doar o sa verific ce topic e pentru a putea cauta in structura de mai sus. Pe langa asta
pot sa am cazul cu wildcard unde sparg stringul in cuvinte si verific daca contine + sau *. Cand contine
o sa intru in structura si o sa sparg si stringul salvat pentru a putea sa tratez cazul de wildcard.

In functia isMatch rezolv problema de a determina daca un topic dintr-o structura este potrivit cu un anumit sablon. 
Am folosit programare dinamica, unde creez o matrice care retine rezultatele partiale ale potrivirii. 
Initializez valoarea matricei la 1 pentru a indica ca ambele siruri sunt goale. 
Apoi, daca topicul incepe cu *, il potrivesc cu un caracter gol, iar celula corespunzatoare este setata la 1.
In cadrul buclei for, tratez doua cazuri: atunci cand topicul contine caracterul * sau +.
Cand intalnesc *, inseamna ca acesta poate fi potrivit cu orice caracter, deci am doua optiuni:
Se potriveste cu caracterul curent din topic si iau valoarea din diagonala stanga-sus.
Nu se potriveste cu caracterul din topic si iau valoarea din celula de sus, reprezentand o secventa mai scurta.
Cand intalnesc +, inseamna ca trebuie sa se potriveasca cu un caracter specific. In acest caz, iau valoarea din diagonala stanga-sus.
In final, functia returneaza ultima valoare inregistrata in matrice, indicand daca topicul este potrivit cu sablonul sau nu.

Dupa daca se returneaza adevarat o sa trimitem pachetul. Aici trebuie trimis ipul si portul corespunzator clientului udp.
Pentru a nu avea probleme de trimitere am preferat sa trimit 2 mesaje de validare si apoi trimiterea ipului si a portului.
Si daca totul este in regula trimit pachetul primit de la udp catre tcp. Daca nu exista match pattern atunci verific doar daca am
topicul bun in structura si trimit.

In ultimul else verificam ce mesaje primesc de la tcp. Daca nu primesc nimic il deconectez pe client.
Daca primesc ceva inseamna ca e mesajul de subscribe unsubscribe, restul nu ma intereseaza.
In subscribe verific dupa id sa vad daca exista, apoi verific daca mai am loc in structura mea sa mai tin topicuri.
Dupa care vad daca deja am dat subscribe si daca am dat deja nu mai pun topicul din nou. Altfel il adaug.
In unsubscribe scot efectiv din structura mea topicul respectiv.

La final de tot inchid toti socketii.

Subscriber.c
Am explicat mai sus in server ca se trimit datele clientului in server. Aici am doar 2 cazuri:
Inputul unde trimit mesajul de subscribe unsubscribe sau exit
Comunicarea cu serverul in care primesc de la el informatii in cazul nostru pachetul de la udp.
Dupa cum e scris in tema am scos topicul, tipul si payloadul si l am afisat. Efectiv scot din buffer
informatiile.

Surse:
https://beej.us/guide/bgnet/html/#poll
https://www.geeksforgeeks.org/wildcard-pattern-matching/
