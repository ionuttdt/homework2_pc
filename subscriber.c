#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s id_client server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	if (argc < 4) {
		usage(argv[0]);
	}
	
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));

	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	int fdmax = sockfd;

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// se trimite mesaj la server
	n = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "send id");

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

			if (FD_ISSET(sockfd, &tmp_fds)) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					memset(buffer, 0, BUFLEN);
					n = recv(sockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");
					if(n == 0)
						break;
					else
						printf("%s\n", buffer);
			}
			else if (FD_ISSET(0, &tmp_fds)){
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					fgets(buffer, BUFLEN - 1, stdin);

					// daca clientul s-a deconectat
					if (strncmp(buffer, "exit", 4) == 0) {
						break;
					}

					// celelalte mesaje si trimit direct serverului care le va traduce
					n = send(sockfd, buffer, strlen(buffer), 0);
					DIE(n < 0, "send");
					memset(buffer, 0, BUFLEN);
				
			}
	}

	close(sockfd);

	return 0;
}
