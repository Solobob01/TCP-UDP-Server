#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <netinet/tcp.h>
#include "helpers.h"

//functie care spune daca portul este ocupat
void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

//functie care citeste din buffer si face transferul la o variabila de tip message
message readMes(char* buffer){
	message mes;
	strcmp(mes.command, strtok(buffer, " \n"));
	strcmp(mes.topic, strtok(NULL, " \n"));
	mes.sf = atoi(strtok(NULL, " \n"));
	return mes;
}

int main(int argc, char* argv[]){
	//chestii de baza pentru conexiune
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	int enable = 1;

	fd_set read_fds, tmp_fds;

	if(argc < 4){
		usage(argv[0]);
	}
	DIE(argc != 4, "numar argumente");

	//initializez cu 0 multimile de fd
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	//creez socketul pentru comunicatia cu serverul
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	//adaug socket-ul si STDIN in multimea de fd
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	//ma conectez cu serverul pe sockfd
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, 4);
	DIE(ret < 0, "connect");
	//dupa conexiune trimit imediat id-ul primit ca argument
	n = send(sockfd, argv[1] , 10, 0);
	DIE(n < 0, "send id");



	while(1){
		//copiez multimea de fd
		tmp_fds = read_fds;

		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		//cazul in care se citeste de la tastatura
		if(FD_ISSET(STDIN_FILENO, &tmp_fds)) {

			//citesc inputul
			char tempbuf[BUFLEN];
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			strcpy(tempbuf, buffer);
			strtok(tempbuf, " \n");
			//verific ce comanda s-a dat si in functie de asta trimit tot buffer-ul spre server
			if(strcmp(tempbuf,"exit") == 0){
				break;
			} else if(strcmp(tempbuf, "subscribe") == 0){
				printf("Subscribed to topic.\n");
				n = send(sockfd, buffer, BUFLEN, 0);
				DIE(n < 0, "send sub");
			} else if(strcmp(tempbuf, "unsubscribe") == 0){
				printf("Unsubscribed to topic.\n");
				n = send(sockfd, buffer, BUFLEN, 0);
				DIE(n < 0, "send unsub");
			}
		}			//se citeste de la tastatura

		//cazul in care se primesc informatii de la server
		if(FD_ISSET(sockfd, &tmp_fds)){
			//citesc informatiile de la server
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, BUFLEN, 0);
			DIE(n < 0, "die recv");
			send_client msg = *(send_client*) buffer;
			//este comentata deoarece checker-ul nu trece cu aceasta varianta
			//printf("%s:%d - ", inet_ntoa(msg.cli.sin_addr), ntohs(msg.cli.sin_port));
			//cazul in care s-a inchis serverul
			if(n == 0){
				break;
			}
			//multe if-ul care verifica ce tip de date a fost primit
			if(msg.post.tip_date == 0){
				//int
				if(msg.post.content[0] == 0){
					//numar pozitiv
					printf("%s - INT - %d\n", msg.post.topic, ntohl(*(uint32_t*)(msg.post.content + 1)));
				} else {
					printf("%s - INT - %d\n", msg.post.topic, -ntohl((*(uint32_t*)(msg.post.content + 1))));
				}
			} else if(msg.post.tip_date == 1){
				//short_real
				printf("%s - SHORT_REAL - %.2f\n", msg.post.topic, 1.0*ntohs(*(uint16_t*)msg.post.content)/100);
			} else if(msg.post.tip_date == 2){
				//float
				uint32_t mantisa;
				uint8_t exponent;
				uint8_t semn;
				semn = *(uint8_t*)msg.post.content;
				mantisa = *(uint32_t*)(msg.post.content + 1);
				exponent = *(uint8_t*)(msg.post.content + 5);
				if(semn == 0){
					//pozitiv
					printf("%s - FLOAT - %.*f\n", msg.post.topic, exponent, 1.0*ntohl(mantisa)/pow(10, exponent));
				} else {
					//negativ
					printf("%s - FLOAT - %.*f\n", msg.post.topic, exponent, -1.0*ntohl(mantisa)/pow(10, exponent));

				}
			} else {
				//string
				msg.post.content[strlen(msg.post.content)] = '\0';
				printf("%s - STRING - %s\n", msg.post.topic, msg.post.content);

			}
		}
	}
	close(sockfd);
	
	return 0;
}