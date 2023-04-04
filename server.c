#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <math.h>
#include <netinet/tcp.h>
#include "helpers.h"

int MAX_CLIENTS = 100;

//functie care iti zice daca portul e folosit sau nu
void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

//functie care returneaza maximul
int getMax(int a, int b){
	return (a > b) ? a : b;
}

//functie care initializeaza toti clientii
client* initClient(){
	client* clients = (client*)calloc(MAX_CLIENTS, sizeof(client));
	for(int i = 0 ;i < MAX_CLIENTS;i++){
		clients[i].nr_topics = 0;
		clients[i].online = 0;
		clients[i].socket = -1;
	}
	return clients;
}

//functie care adauga un client in vectorul de clienti
client* addClient(client* clients,int* noClients,int newSock, char* buffer){
	//in buffer este stocat id-ul clientului
	for(int i = 0;i < *noClients;i++){
		//cautam clientul cu id-ul primit ca argument
		if(strcmp(clients[i].id, buffer) == 0){
			//in caz ca l-am gasit atunci ii setam nou socket si il facem online
			if(clients[i].online == 0){
				//am gasit clientu
				clients[i].socket = newSock;
				clients[i].online = 1;
				return clients;
			//altfel exista un client cu id-ul dat ca argument asa ca dam exit
			} else {
				//exista unu
				close(newSock);
				printf("Client %s already connected.\n", clients[i].id);
				return NULL;
			}
		}
	}
	//cazul in care nu am gasit clientul cu id-ul cerut astfel adaugam un nou client
	//cazul in care se ajuge la numarul maxim de clienti
	if(*noClients >= MAX_CLIENTS){
		MAX_CLIENTS *= 2;
		clients = realloc(clients, MAX_CLIENTS * sizeof(client));
	}
	strcpy(clients[*noClients].id, buffer);
	clients[*noClients].socket = newSock;
	clients[*noClients].online = 1;
	(*noClients)++;
	return clients;
}

//functie care adauga topic pentru clientul specific
client* addTopic(client* clients, int nrClients, int socket, char* topicName, int sf){

	for(int i = 0;i < nrClients;i++){
		if(clients[i].socket == socket){
			for(int j = 0;j < clients[i].nr_topics;j++){
				//se verifica daca exista deja topicul vechi
				if(strcmp(clients[i].topics[j].topic, topicName) == 0){
					//astfel se seteaza status-ul ca fiind 1 si daca acesta doreste SF pentru acest topic
					clients[i].topics[j].status = 1;
					clients[i].topics[j].sf = sf;
					return clients;
				}
			}
			//altfel se creaza un topic nou pentru clientul specific
			strcpy(clients[i].topics[clients[i].nr_topics].topic, topicName);
			clients[i].topics[clients[i].nr_topics].status = 1;
			clients[i].topics[clients[i].nr_topics].sf = sf;
			clients[i].topics[clients[i].nr_topics].nr_msg_saved = 0;
			clients[i].nr_topics++;
			break;
		}
	}
	return clients;
}

//functie care scoate topicul de la un client specific
client* removeTopic(client* clients, int nrClients, int socket, char* topicName){
	for(int i = 0;i < nrClients;i++){
		if(clients[i].socket == socket){
			for(int j = 0;j < clients[i].nr_topics;j++){
				//cazul in care am gasit topicul in vectorul de topicuri al clientului
				if(strcmp(clients[i].topics[j].topic, topicName) == 0){
					//seteaza statusul ca fiind 0
					clients[i].topics[j].status = 0;
					return clients;
				}
			}
		}
	}
	return clients;
}


//functie care adauga mesaje primite cand clientul este offline in vectorul de mesaje vechi
client* addToLater(client* clients,int pos, char* buffer, int topic){
	//printf("buf:%s\n", buffer);
	memcpy(clients[pos].topics[topic].msg_saved[clients[pos].topics[topic].nr_msg_saved], buffer, BUFLEN);
	clients[pos].topics[topic].nr_msg_saved++;
	return clients;
}

//functie care trimite informatiile despre topic la toti clientii din baza de date
void sendTopicToClients(client* clients, int nrClients, char* buffer, send_client msg){
	for(int i = 0;i < nrClients;i++){
		for(int j = 0;j < clients[i].nr_topics;j++){
			if(strcmp(clients[i].topics[j].topic, msg.post.topic) == 0 && clients[i].topics[j].status){
				//daca acesta doreste sa pastreze mesajele si nu este online
				if(clients[i].topics[j].sf && !clients[i].online){
					//printf("se adauga mai tarziu\n");
					clients = addToLater(clients, i, (char*)&msg, j);
					continue;
				} else {
					//daca acesta nu doreste sa pastreze mesajele dar este online atunci le trimit direct
					if(clients[i].online){
						int n = send(clients[i].socket, (char*)&msg, BUFLEN, 0);
						DIE(n < 0, "send o && sf");
						continue;
					}
				}
			}
		}
	}
}

//functie care trimite toate mesajele vechi ale unui client atunci cand acesta se conecteaza
void sendAllOldMes(client* clients, int nrClients,int socket){
	for(int i = 0;i < nrClients;i++){
		if(clients[i].socket == socket){
			for(int j = 0;j < clients[i].nr_topics;j++){
				if(clients[i].topics[j].status){
					if(clients[i].topics[j].sf){
						for(int k = 0;k < clients[i].topics[j].nr_msg_saved;k++){
							int n = send(clients[i].socket, clients[i].topics[j].msg_saved[k],
								BUFLEN, 0);
							DIE(n < 0, "send old msg");
						}
						clients[i].topics[j].nr_msg_saved = 0;
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]){
	//chestii de baza pentru conexiune
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	char buffer[BUFLEN];
	int counterClient = 0;
	client* clients = initClient();
	struct sockaddr_in serv_addr, newcli_addr;
	socklen_t clilen;
	int n, ret;
	int tcp_sock, udp_sock, newsockfd;
	int tcp_listen;
	int fdmax;
	int portno;
	int enable;

	fd_set read_fds;
	fd_set tmp_fds;

	if(argc < 2){
		usage(argv[0]);
	}

	//initializez cu 0 multimile fd
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	//initializez un socket pentru tcp
	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock < 0, "tcp_sock");

	//initializez un socket pentru udp
	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_sock < 0, "udp_sock");
	setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, &enable, 4);


	portno = atoi(argv[1]);
	DIE(portno == 0, "port number");

	//initializez adresa serverului
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(tcp_sock, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "tcp bind");
	ret = bind(udp_sock, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "udp bind");

	//verific daca pot sa ascult pe tcp
	tcp_listen = listen(tcp_sock, INT_MAX);
	DIE(tcp_listen < 0, "tcp listen");

	//adauga cei 3 socketi in multimea de fd
	FD_SET(tcp_sock, &read_fds);
	FD_SET(udp_sock, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	//aleg maximul dintre socketul de tcp si cel de udp
	fdmax = getMax(tcp_sock, udp_sock);

	while(1){
		//clonez read_fd
		tmp_fds = read_fds;

		//selectez toti socketi pe care s-au fct modificari
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for(int i = 0;i <= fdmax;i++){
			if(FD_ISSET(i, &tmp_fds)){
				//verific daca socket-ul este tcp
				if(i == tcp_sock){
					//un client nou vrea sa se conecteze
					clilen = sizeof(struct sockaddr);
					newsockfd = accept(tcp_sock, (struct sockaddr *) &newcli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					//actualizez maximul
					FD_SET(newsockfd, &read_fds);
					//elimin algoritmul lui Nagle de pe socket
					setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &enable, 4);
					fdmax = getMax(newsockfd, fdmax);

					//citesc id-ul clientului
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, 10, 0);
					DIE(n < 0, "recv");
					//adaug clientul in vectorul de clienti
					client* tempcli  = addClient(clients, &counterClient,newsockfd, buffer);
					//cazul NULL inseamna ca clientul exista in baza de date
					if(tempcli == NULL){
						FD_CLR(newsockfd, &read_fds);
						continue;
					}
					//actualizez baza de date
					clients= tempcli;
					printf("New client %s connected from %s:%d\n",buffer,
							inet_ntoa(newcli_addr.sin_addr), ntohs(newcli_addr.sin_port));
					//trimit toate mesajele vechi acestui client
					sendAllOldMes(clients, counterClient, newsockfd);
				//verific daca socket-ul este udp
				} else if (i == udp_sock) {
					//citesc topicul nou de pe udp_sock
					clilen = sizeof(newcli_addr);
					memset(buffer, 0, BUFLEN);
					n = recvfrom(i, buffer, BUFLEN, 0, (struct sockaddr*)&newcli_addr, &clilen);
					DIE(n < 0,"recv udp");
					//trimit acest topic alaturi de adresa ip si port-ul clientului care este abonat la topic
					send_client msg;
					msg.post = *(udp_post*) buffer;
					msg.cli = newcli_addr;
					sendTopicToClients(clients, counterClient, buffer, msg);

				//verific daca se citeste de la tastatura
				} else if(i == STDIN_FILENO){
					//se citeste de la tastatura
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv STDIN");
					//in cazul in care este "exit" inchid toate conexiunile
					if(strcmp(strtok(buffer, " \n"), "exit")){
						for(int j = 0; j <= fdmax;j++){
							close(j);
							FD_CLR(j, &read_fds);
						}
						break;
					}
				//cazul in care un client trimite comenzi la server
				} else {
					//citesc comanda de la server
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");
					//daca este 0 inseamna ca acesta s-a deconectat
					if(n == 0){
						for(int c = 0;c < counterClient;c++){
							if(i == clients[c].socket){
								clients[c].online = 0;
								printf("Client %s disconnected.\n", clients[c].id);
								close(i);
								FD_CLR(i, &read_fds);
								continue;
							}
						}
					} else {
						//altfel citesc comanda
						char tmp[BUFLEN];
						strcpy(tmp, buffer);
						message mes;
						strcpy(mes.command, strtok(tmp, " \n"));
						strcpy(mes.topic, strtok(NULL, " \n"));
						//daca este de unsubscribe sterg topicul din lista de topic a clientului doritor
						if(strcmp(mes.command, "unsubscribe") == 0){
							removeTopic(clients, counterClient, i , mes.topic);
						//altfel adaug topicul in lista de topic a clientului doritor
						} else if(strcmp(mes.command, "subscribe") == 0){
							mes.sf = atoi(strtok(NULL, " \n"));
							addTopic(clients, counterClient, i , mes.topic, mes.sf);
						}
					}
				}
				
			}
		}
	}
	free(clients);
	close(tcp_sock);
	close(udp_sock);
	return 0;
}

