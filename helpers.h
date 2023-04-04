#ifndef _HELPERS_H
#define _HELPERS_H 1
#define BUFLEN 1600

#pragma pack(1)

//functie care da kill la aplicatie in caz de eroare
#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


//salveaza commanda de la client
typedef struct messsage{
	//comanda
	char command[50];
	//topicul la care vrem sa ne abonam
	char topic[50];
	//daca vrei sau nu sf
	unsigned int sf;
}message;

//structura care salveaza un topic
typedef struct topic{
	//numele topicului
	char topic[50];
	//numarul de mesaje salveaza pentru acest topic
	int nr_msg_saved;
	//vectorul de stringul care contine mesajele salvate
	char msg_saved[50][2000];
	//daca clientul x este abonat sau nu la acest topic
	unsigned int status;//0 - NU, 1 - DA
	//daca se doreste sf pe acest topic
	unsigned int sf;
}topic;

//structura care salveaza inputul de la un mesaj de tip udp
typedef struct udp_post{
	//numele topicului
	char topic[50];
	//tipul de date primit
	unsigned char tip_date;
	//continului mesajului
	char content[BUFLEN];
}udp_post;

//structura pentru clientii abonati
typedef struct client{
	//numele clientului(id)
	char id[50];
	//numarul total topicuri la care s-a abonat
	int nr_topics;
	//vectorul cu topicuri
	topic topics[100];
	//socketul asociat
	int socket;
	//starea daca este online sau nu
	int online; //0 - NU, 1 - DA
}client;

//structura cu informatiile ca sunt trimise la clienti
typedef struct send_client{
	//adresa de unde a venit informatia
	struct sockaddr_in cli;
	//informatia trimisa
	struct udp_post post;
}send_client;
#endif