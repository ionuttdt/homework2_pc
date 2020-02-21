/*
	Duta Viorel-Ionut
	Tema 2 PC
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "helpers.h"



void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	int sockfd_udp;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	struct sockaddr_in udp_addr;			//pentru udp
	int n, i, ret;
	socklen_t clilen, clilen_udp;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	AList clienti = NULL;
	char *buffaux = NULL;

	// daca nu primesc portul
	if (argc < 2) {
		usage(argv[0]);
	}

	// Socketi UDP si TCP:

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// portul dat ca parametru
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi portno");

	// adaug client TCP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	ret = listen(sockfd, SOMAXCONN);
	DIE(ret < 0, "listen");

	// adaug client UDP
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket");


	memset((char *) &udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd_udp, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");


	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	FD_SET(sockfd_udp, &read_fds);
	fdmax = sockfd_udp;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept tcp");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					memset(buffer, 0, BUFLEN);

					n = recv(newsockfd, buffer, BUFLEN, 0);
					DIE(n < 0, "ID Client");
					
					char idaux[10];				// id-ul clientului conectat
					strcpy(idaux, buffer);

					// verific daca e prima data cand acest client se conecteaza folosind id-ul
					if(client_exit(clienti,idaux)) {
						AList aux = add_client(clienti, buffer, ntohs(cli_addr.sin_port), newsockfd);
						if(aux == NULL)
							DIE(-1, "malloc");
						clienti = aux;
						printf("New client %s connected from %s:%d.\n",			// mesaj doar pentru clientii noi
							clienti->info->id_client,inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					}
					// daca nu e prima data il setez iar ca fiind conectat
					else {
						conectare_client(clienti, idaux);
					}

				}
				// primesc mesaj de la un client UDP
				else if(i == sockfd_udp) {
					n = recvfrom(sockfd_udp, buffer, BUFLEN, 0, (struct sockaddr*) &udp_addr, &clilen_udp );
					DIE(n < 0, "recvfrom udp");
					
					// construiesc IP:PORT in buffer aux
					char portnr[4] ;
					sprintf(portnr,"%d",ntohs(udp_addr.sin_port));
					buffaux = concat(inet_ntoa(udp_addr.sin_addr),":");
					buffaux = concat(buffaux, portnr);
					buffaux = concat(buffaux, " - ");

					char buff_topic[50];
					char buff_tip = 0;

					sscanf(buffer, "%s", buff_topic);
					sscanf(buffer + 50, "%c", &buff_tip);

					buffaux = concat(buffaux, buff_topic);
					buffaux = concat(buffaux, " - ");

					// transform informatia intr-un format pe care clientii sa il inteleaga
					if((int) buff_tip == 0) {

						int int_nr;
						char int_to_char[4];
						char semn_aux = 0;
						buffaux = concat(buffaux, "INT");
						buffaux = concat(buffaux, " - ");

						sscanf(buffer + 51, "%c", &semn_aux);
						int_nr = ntohl( *(int*)(buffer + 52));

						sprintf(int_to_char, "%d", int_nr);
						if(semn_aux == 1)
							buffaux = concat(buffaux, "-");

						buffaux = concat(buffaux, int_to_char);
						send_tcp(clienti, buff_topic, buffaux);

					}
					else if((int) buff_tip == 1) {
						buffaux = concat(buffaux, "SHORT_REAL");
						buffaux = concat(buffaux, " - ");

						double short_to_float =(double) ntohs(*(short*)(buffer + 51));
						short_to_float = short_to_float/100;

						char ans_short[5];
						sprintf( ans_short, "%0.2f", short_to_float);

						buffaux = concat(buffaux, ans_short);
						send_tcp(clienti, buff_topic, buffaux);
					}
					else if((int) buff_tip == 2) {
						buffaux = concat(buffaux, "FLOAT");
						buffaux = concat(buffaux, " - ");

						char semn_float = 0;
						int nr_float = 0;
						char pow_float = 0;
						int pow_float2;
						char ans_float[9];

						sscanf(buffer + 51, "%c", &semn_float);
						nr_float = ntohl( *(int*) (buffer + 52));
						pow_float =  *(char*) (buffer + 56);
						pow_float2 = pow_float;

						if(semn_float == 1)
							buffaux = concat(buffaux, "-");
						float ans_float2 = div_float(nr_float, pow_float2);
						sprintf(ans_float, "%.*f",pow_float2 , ans_float2);

						buffaux = concat(buffaux, ans_float);
						send_tcp(clienti, buff_topic, buffaux);

					}
					else {
						buffaux = concat(buffaux, "STRING");
						buffaux = concat(buffaux, " - ");

						char ans_str[1500];
						memset(ans_str, 0, 1500);
						sscanf(buffer + 51, "%s", ans_str);

						buffaux = concat(buffaux, ans_str);
						send_tcp(clienti, buff_topic, buffaux);
					}					
					
					// golire buffer si bufferaux
					memset(buffaux, 0, strlen(buffaux));
					memset(buffer, 0, BUFLEN);
				}
				// daca serverul a primit un mesaj de la tastatura (exit)
				else if(i == 0) {								
					//inchid serverul pt exist
					memset(buffer, 0, BUFLEN);

					fgets(buffer, BUFLEN - 1, stdin);

					if (strncmp(buffer, "exit", 4) == 0) {
						return 0;
					}
				}
				// s-a primit un mesaj de la un client TCP deja conectat
				else {
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					// clientul s-a deconectat de la server si get_id_client il marcheaza ca si deconectat
					if (n == 0) {
						char *aux_id = get_id_client(clienti, i);
						printf("Client %s disconnected.\n", aux_id);					
						close(i);
						FD_CLR(i, &read_fds);
					}
					// cazul subscribe/unsubscribe
					else {
						if(strncmp(buffer, "subscribe", 9) == 0) {
							char *token = strtok(buffer, " ");
							token = strtok(NULL, " ");
							char *topaux = token;
							token = strtok(NULL, " ");
							int sfaux = atoi(token);
							subscribe(clienti, i, topaux, sfaux);	// se adauga un nou topic clientului specificat

						}
						else if(strncmp(buffer, "unsubscribe", 11) == 0) {
							char *token = strtok(buffer, " ");
							token = strtok(NULL, " ");
							unsubscribe(clienti, i, token);			// se sterge topicul respectiv din lista
						}
						//un mesaj care nu are nicio relevanta pentru server
						else {				
							printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
						}
					}
				}
			}
		}
	}

	close(sockfd);
	close(sockfd_udp);

	return 0;
}
