#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

// structuri topicuri client
typedef struct storef {
	char *buf;				// informatiile neprimite inca
	struct storef *urm;
} S_F, *ASF;

typedef struct topic {
	int SF;					// store&foreward
	char nume_topic[50];	// numele topicului
	struct topic *urm;		// lista celorlalte topicuri
	ASF mesaje_sf;			// mesajele netrimise inca
} LTopic, *ALT;

// structuri lista de clienti:
typedef struct clientTCP {
	char* id_client;		// ID-ul unic
	int port_client;		// portul pe care s-a conectat
	int socket_client;		// socketul
	int c_d;				// 1 daca e conectat si 0 daca nu e
	ALT ltopic;				// lista topicurilor la care este abonat
} CTCP, *ATCP;

typedef struct list {
	ATCP info;
	struct list *urm;
} List, *AList, **AAList;

// construiesc un nou client cu un id, port si socket dat
AList allocL(char* id, int port, int socket) {
	AList aux = malloc(sizeof(List));
	if(!aux)
		return NULL;
	aux->info = malloc(sizeof(CTCP));
	if(aux->info == NULL) {
		free(aux);
		return NULL;
	}
	aux->info->id_client = calloc(11, sizeof(char));
	if(!(aux->info->id_client)) {
		free(aux);
		return NULL;
	}
	memcpy(aux->info->id_client, id, 10);
	aux->info->port_client = port;
	aux->info->socket_client = socket;
	aux->info->c_d = 1;
	aux->info->ltopic = NULL;
	aux->urm = NULL;

	return aux;
} 

// constuieste un nou topic 
ALT add_topic(int sf, char *numetopic) {
	ALT aux = malloc(sizeof(LTopic));
	if(!aux)
		return NULL;
	aux->urm = NULL;
	aux->mesaje_sf = NULL;
	aux->SF = sf;
	strcpy(aux->nume_topic, numetopic);
	return aux;
}

// adaug un nou client la inceputul listei
AList add_client(AList l, char* id, int port, int socket) {
	AList aux = NULL;
	aux = allocL(id, port, socket);
	if(aux == NULL)
		return NULL;
	aux->urm = l;
	return aux;
}

// parcurg lista de clienti pana il gasesc pe cel care are un anumit socket
// functie folosita la disconect, deci setez si l->info->c_d = 0
char* get_id_client(AList clienti, int socket) {
	AList l = clienti;
	while(l != NULL) {
		if(l->info->socket_client == socket) {
			l->info->c_d = 0;				
			return l->info->id_client;
		}
		l = l->urm;
	}
	return NULL;
}

// 0 daca un client s-a autentificat si 1 daca nu
int client_exit(AList clienti, char* id) {
	AList aux = clienti;
	while(aux != NULL) {
		if(*(aux->info->id_client) == *id )
			return 0;
		aux = aux->urm;
	}
	return 1;
}

// cand reconectez un client
void conectare_client(AList clienti, char* id) {
	AList aux = clienti;
	while(aux != NULL) {
		if(*(aux->info->id_client) == *id ) {
			aux->info->c_d = 1;					// ii setez stare iar la conectat
			// trimit mesajele stocate in lista mesaje_sf cat timp clientul nu a fost conectat 
			ALT aux_topic = aux->info->ltopic;
			while(aux_topic != NULL) {
				ASF aux_sf = aux_topic->mesaje_sf;
				while(aux_sf != NULL) {
					//send(aux->info->socket_client, buffer, strlen(buffer), 0);
					ASF aux_sf_free = aux_sf;
					send(aux->info->socket_client, aux_sf->buf, strlen(aux_sf->buf), 0);
					aux_sf = aux_sf->urm;
	
					free(aux_sf_free->buf);
					free(aux_sf_free);
	
				}
				aux_topic->mesaje_sf = NULL;
				aux_topic = aux_topic->urm;
			}
			
		}
		aux = aux->urm;
	}
}

// caut clientul dupa socket si dupa ii adaug un nou topic in lista
// ltopic
void subscribe(AList clienti, int socket, char* topic, int sf) {
	AList C = clienti;
	while(C != NULL) {
		if(C->info->socket_client == socket) {
			ALT aux = add_topic(sf, topic);
			if(aux == NULL)
				return;
			aux->urm = C->info->ltopic;
			C->info->ltopic = aux;
			aux->mesaje_sf = NULL;
			return;
		}
		else {
			C = C->urm;
		}
	}
}

// elimin un topic din lista clientului
void unsubscribe(AList clienti, int socket, char *topic) {
	AList aux = clienti;
	while(aux != NULL) {
		if(aux->info->socket_client == socket) {
			ALT aux2 = aux->info->ltopic;
			ALT aux3 = NULL;
			while(aux2 != NULL) {
				if( strncmp(aux2->nume_topic, topic, strlen(topic)-1) == 0 ) {
					if(aux3 == NULL) {
						aux->info->ltopic = aux2->urm;
						return;
					}
					aux3->urm = aux2->urm;
					return;

				}
				aux3 = aux2;
				aux2 = aux2->urm;
			} 
		}
		aux = aux->urm;
	}
}

// parcurg intreaga structura si daca un client e abonat la topicul dat ii trimit
// buffer (mesaj construit in server)
void send_tcp(AList clienti, char *topic, char *buffer) {
	AList aux = clienti;
	while(aux != NULL) {
		ALT aux_topic = aux->info->ltopic;
		while(aux_topic != NULL) {
			if( strncmp(aux_topic->nume_topic, topic, strlen(topic)) == 0 ) {
				if(aux->info->c_d == 1)
					send(aux->info->socket_client, buffer, strlen(buffer)+1, 0);
				else if(aux_topic->SF == 1){
					ASF aux_sf = aux_topic->mesaje_sf;
					if(aux_sf == NULL) {
						aux_topic->mesaje_sf = malloc(sizeof(S_F));
						aux_topic->mesaje_sf->buf = calloc(1500,sizeof(char));
						aux_topic->mesaje_sf->urm = NULL;
						memcpy(aux_topic->mesaje_sf->buf, buffer, strlen(buffer)+1);
					}
					else{
						while(aux_sf->urm != NULL) {
							aux_sf = aux_sf->urm;
						}
						aux_sf = malloc(sizeof(S_F));
						aux_sf->buf = calloc(1500,sizeof(char));
	
						aux_sf->urm = NULL;
						memcpy(aux_sf->buf, buffer, strlen(buffer)+1);
					}	
				}
			}
			aux_topic = aux_topic->urm;
		}
		aux = aux->urm;
	}
}

// functie pentru concatenarea a doua stringuri
char* concat(const char *s1, const char *s2) {
    char *r = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(r, s1);
    strcat(r, s2);
    return r;
}

// functie folosita pentru parsarea unui FLOAT
float div_float(int n, int k) {
	int i ;
	float r = (double) n;
	for(i = 0; i < k; i++)
		r = r/10;
	return r;
}

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

/* Dimensiunea maxima a calupului de date */
#define BUFLEN 1500

#endif
