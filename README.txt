Tema realizata de Craciun Alexandru-Andrei 323CD

Am folosit 2 sleep-days.

In Makefile am introdus si o comanda "make run_test" care ruleaza testul.

Structurile folosite in helper.h:
    message: are ca scop sa memoreze o comanda tastata de un utilizator. Se memoreaza:
        numele comenzii
        topic la care vrem sa ne abonam / dezabonam
        daca dorim sau nu sf

    topic: are ca scop sa mememoreze diferite informatii legate de topic:
        numele acestuia
        numarul de mesaje salvate si toate mesajele salvate
        daca acest topic aste activ sau nu in cadrul unui client
        daca un client doreste sau nu sf pe topic
    
    udp_post: are ca scop sa salveze un mesaj primit de la un client udp. Se salveaza:
        topicul asocitat mesajului
        tipul de date care se afla in mesaj
        continutui propriu zis
    
    client: are ca scop sa memoreze informatiile legate de un client acestea fiind:
        id acestuia
        numarul de topicul la care s-a abonat si topicurile aferente
        socketul asociat clientului
        daca este online sau nu
    
    send_client: are ca scop sa formateze frumos datele care sunt trimise catre abonati.Acesta contine:
        informatiile legate de clientul de pe care s-a trimis topicul
        informatiile care sunt trimise catre abonat

Subscriber:
    Initial se realizeaza conexiunea cu server-ul. Dupa se verifica pe cazuri daca se citeste de la tastatura
        sau se primesc informatii de la server.
    In cazul in care se citeste de la tastatura, in functie de comanda ori inchidem programul ori trimitem informatiile
        primite la input catre server.
    In cazul in care se primesc informatii de la server, acestea se formateaza astfel incat sa fie human-readable si se
        afiseaza la consola.

Server:
    Initial se deschide un socket pentru clientii tcp si altul pentru clientii udp.
    Folosind un vector alocat dinamic memoram clientii care sunt conectati la server.
    Verificam pe ce socket se primesc informatii.
    In cazul in care este cel de tcp atunci un client vrea sa se conecteze la server. Astfel il adaugam in vectorul
        de clienti si trimitem toate informatiile pe care a dorit sa le primeasca(are setat sf ca 1).
    In cazul in care este cel de udp atunci cineva vrea sa trimita un topic si informatii legate de acel topic. Astfel
        informatia primita este trimisa la toti clienti din baza de date care sunt abonati la acest topic primit(si sunt
        online).
    In cazul in care este cel de STDIN atunci citim verificam daca inputul este cel de "exit". Astfel inchidem programul
        si trimitem un mesaj null la toti clientii astfel sa ii anuntam ca serverul s-a inchis si sa-si inchida si ei programul.
    Altfel un client incearca sa trimita comenzi la server.