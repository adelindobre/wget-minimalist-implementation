#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <unistd.h>     
#include <sys/types.h>  
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <stdbool.h>

#define HTTP 80
#define MAXLEN 500

/**
 * Citeste maxim maxlen octeti din socket-ul sockfd. Intoarce
 * numarul de octeti cititi.
 */

/*GLOBAL VAR*/
FILE *flog;
char server_name[50];
struct sockaddr_in servaddr;
char cwd[1024];
const char extensions[50] = ".png .zip .pdf .txt .ico .php";
char init_path[1024];
char comenzi[MAXLEN];

ssize_t Readline(int sockd, char *buffer, size_t maxlen) 
{
    ssize_t n, rc;
    char    c;

    for ( n = 1; n < maxlen; n++ ) 
    {	
		if ( (rc = read(sockd, &c, 1)) == 1 ) 
		{
	    	*buffer++ = c;
	    	if ( c == '\n' )
			break;
		}
		else if ( rc == 0 ) 
		{
    		if ( n == 1 )
				return 0;
	    	else
				break;
		}
		else 
		{
		    if ( errno == EINTR )
				continue;
		    return -1;
		}
    }

    *buffer = 0;
    return n;
}
/**
 * Trimite o comanda SMTP si asteapta raspuns de la server.
 * Comanda trebuie sa fie in buffer-ul sendbuf.
 * Sirul expected contine inceputul raspunsului pe care
 * trebuie sa-l trimita serverul in caz de succes (de ex. codul
 * 250). Daca raspunsul semnaleaza o eroare se iese din program.
 */
void send_command(int flag_e, FILE *fp, int sockfd, char sendbuf[], char *expected)
{
	char recvbuf[MAXLEN];
	int nbytes;
	char CRLF[3];

	CRLF[0] = 13; CRLF[1] = 10; CRLF[2] = 0;
	strcat(sendbuf, CRLF);
	write(sockfd, sendbuf, strlen(sendbuf));

	if (flag_e == 1)
		nbytes = read(sockfd, recvbuf, MAXLEN - 1);
	else
		nbytes = Readline(sockfd, recvbuf, MAXLEN - 1);

	if (strstr(recvbuf, expected) != NULL)
	{
		if (strstr(comenzi, "-o") != NULL)
			fprintf(flog, "%s", "Am primit mesaj de eorare de la server!\n");
		else
	 		printf("Am primit mesaj de eorare de la server!\n");
	 	exit(-1);
	}

	int flag ;
	
	if (flag_e == 1)
		flag = 1;
	else
		flag = 0;

	while(nbytes > 0)
	{		
		if (flag_e == 1 && strstr(recvbuf, "\r\n\r\n") != NULL && flag == 1)
		{
			char *recv = malloc(MAXLEN * sizeof(char));
			strcpy(recv, strstr(recvbuf, "\r\n\r\n"));
			recv = recv + strlen("\r\n\r\n");
			fwrite(recv, strlen(recvbuf), 1, fp);
			flag = 0;
			continue;				
		}
		if (flag == 0)
		{
			if (flag_e == 0)
				recvbuf[nbytes] = 0;
			if (flag_e == 1)
				fwrite(recvbuf, nbytes, 1, fp);
			else
				fprintf(fp, "%s", recvbuf);		
		}
		if (flag_e == 1)
			nbytes = read(sockfd, recvbuf, MAXLEN - 1);
		else
			nbytes = Readline(sockfd, recvbuf, MAXLEN - 1);

	}
}

void executeCommands(char* search_file, char comanda[], char sendbuf[], int sockfd);

void createDirectories(int flag_e, char *file_name, int server_state, char comanda[], char sendbuf[], int sockfd)
{
	printf("%s\n", file_name);
	char *file_aux = malloc(100*sizeof(char));
	char *subdir;
	char *dir = malloc(100*sizeof(char));
	strcpy(file_aux, file_name);	

	if (server_state == 1)
	{
		strcpy(comanda, "mkdir ");
		strcat(comanda, server_name);
		system(comanda);
		strcpy(comanda, server_name);
		chdir(comanda);
	}
	memset(comanda, 0, 50);

	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		strcpy(cwd, strrchr(cwd,'/') + 1);
		
		if (strchr(file_aux, '/') != NULL)
		{
			subdir = strchr(file_aux, '/');
			strncpy(dir, file_aux, (strlen(file_aux) - strlen(subdir)));
			
			if (strcmp(dir,"..") == 0)
				chdir("..");
			else	
				if (strcmp(cwd, dir) != 0)
				{
					strcpy(comanda, "mkdir ");
					strcat(comanda, dir);			
					system(comanda);
					strcpy(comanda, dir);
					chdir(comanda);	
					memset(comanda, 0, 50);			
				}
			strcpy(file_aux, (file_aux + strlen(dir) + 1));
//			printf("%s\n", file_aux);					
		}						
	}
	else
		if (strstr(comenzi, "-o") != NULL)
			fprintf(flog, "%s", "getcwd() error");
		else
			perror("getcwd() error");

	memset(dir, 0, 100);
	memset(cwd, 0, 1024);

	while ((subdir = strchr(file_aux, '/')) != NULL)
	{
		strncpy(dir, file_aux, (strlen(file_aux) - strlen(subdir)));
//		printf("%s\n", dir);
		if (strcmp(dir,"..") == 0)
			chdir("..");
		else
		{
			strcpy(comanda, "mkdir ");
			strcat(comanda, dir);
			system(comanda);
			strcpy(comanda, dir);
			chdir(comanda);	
		}		
		strcpy (file_aux, (file_aux + strlen(dir) + 1));
//		printf("%s\n", file_aux);
		memset(dir, 0, 100);
		memset(comanda, 0, 50);		
	}	
	memset(comanda, 0, 50);
	memset(dir, 0, 100);		

	FILE *file_p;
	if (flag_e == 1)
		file_p = fopen(file_aux, "wb");
	else
		file_p = fopen(file_aux, "w");

	memset(sendbuf, 0, MAXLEN);
	strcpy(sendbuf,"GET /");	
  	strcat(sendbuf, file_name);
  	strcat(sendbuf, "/ HTTP/1.0\nHOST: ");
  	strcat(sendbuf, server_name);
  	strcat(sendbuf, "\n");

  	if (flag_e == 1)
		send_command(1, file_p, sockfd, sendbuf, "HTTP/1.0 200 OK");
	else
		send_command(0, file_p, sockfd, sendbuf, "HTTP/1.0 200 OK");

	fclose(file_p);	

	if (server_state == 1)
		strcpy(init_path, getcwd(cwd, sizeof(cwd)));

	if (flag_e == 1)
	{
		chdir(init_path);
	}
	printf("%s\n", file_aux);
	if (flag_e == 0)
		executeCommands(file_aux, comanda, sendbuf, sockfd);	
}

bool findFileExtension(char *path_to_file)
{
	char *token = malloc(100 * sizeof(char));
	char *copy_ext = strdup(extensions);
	
	token = strtok(copy_ext, " ");
	while (token != NULL)
	{
		if (strstr(path_to_file, token) != NULL)
			return true;
		token = strtok(NULL, " ");
	}
	return false;
}

//executeCommands(comenzi, search_file, command, sendbuf, sockfd);
void executeCommands(char* search_file, char comanda[], char sendbuf[], int sockfd)
{
//	printf("%s\n", search_file);
	FILE *fp;
	char *buffer = malloc(1000 * sizeof(char));
	fp = fopen(search_file, "r");

	if (!fp)
	{
		if (strstr(comenzi, "-o") != NULL)
			fprintf(flog, "%s", "Fisierul nu se poate deschide\n");
		else
			printf("Fisierul nu se poate deschide\n");
		exit(-1);
	}

	char *link = malloc(1000 * sizeof(char));
	char *end_link = malloc(1000 * sizeof(char));
	char *tag = malloc(1000 * sizeof(char));
	char *path_to_file = malloc(1000 * sizeof(char));
	char *start_path = malloc(1000 * sizeof(char));
	char *end_path = malloc(1000 * sizeof(char));
	
	while (fgets(buffer, 1000, fp) != NULL)
	{
		if (strstr(buffer, "<a") != NULL)
		{
			strcpy(link, strstr(buffer, "<a"));
			if (strstr(link, "href") == NULL)
			{
				memset(buffer, 0, 1000);
				memset(link, 0, 1000);
				continue;
			}
			if (strchr(link,'>') == NULL)
			{
				memset(buffer, 0, 1000);
				memset(link, 0, 1000);
				continue;	
			}
			strcpy(end_link, strchr(link, '>'));
			strncpy(tag, link, (strlen(link) - strlen(end_link)));

			if (strstr(tag, "href=\"") != NULL)
			{
				strcpy(start_path, strstr(tag, "href=\""));
				start_path = start_path + strlen("href=\"");
			}
			else
			{
				strcpy(start_path, strstr(tag, "href='"));
				start_path = start_path + strlen("href='");	
			}
			if (strchr(start_path, '#') != NULL)
			{
				memset(buffer, 0, 1000);
				memset(link, 0, 1000);
				memset(end_link, 0, 1000);
				memset(tag, 0, 1000);
				memset(start_path, 0, 1000);
				continue;
			}

			if (strchr(start_path, '"') != NULL)
				strcpy(end_path, strchr(start_path, '"'));
			else
				strcpy(end_path, strchr(start_path, '\''));

			strncpy(path_to_file, start_path, (strlen(start_path) - strlen(end_path)));
		}
		else
			if (strstr(buffer, "</a>") != NULL)
			{
				if (strstr(buffer, "href") == NULL)
				{
					memset(buffer, 0, 1000);
					memset(link, 0, 1000);
					continue;
				}
				if (strstr(buffer, "href=\"") != NULL)
				{
					strcpy(start_path, strstr(buffer, "href=\""));
					start_path = start_path + strlen("href=\"");
				}
				else
				{
					strcpy(start_path, strstr(buffer, "href='"));
					start_path = start_path + strlen("href='");	
				}
				if (strchr(start_path, '#') != NULL)
				{
					memset(buffer, 0, 1000);
					memset(link, 0, 1000);
					memset(end_link, 0, 1000);
					memset(tag, 0, 1000);
					memset(start_path, 0, 1000);
					continue;
				}

				if (strchr(start_path, '"') != NULL)
					strcpy(end_path, strchr(start_path, '"'));
				else
					strcpy(end_path, strchr(start_path, '\''));

				strncpy(path_to_file, start_path, (strlen(start_path) - strlen(end_path)));
			}
			

		if (strstr(comenzi, "-e") != NULL)
		{
			if (findFileExtension(path_to_file) == true)
			{
				close(sockfd);
				/*	asignare socket-ul	*/							
			  	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
			  	{
			  		if (strstr(comenzi, "-o") != NULL)
						fprintf(flog, "%s", "Eroare la creare socket\n");
					else
						printf("Eroare la  creare socket.\n");
					exit(-1);
				} 
					/* conectare la server */
			 	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
			 	{
			 		if (strstr(comenzi, "-o") != NULL)
						fprintf(flog, "%s", "Eroare la conectare\n");
					else
				    	printf("Eroare la conectare\n");
				    exit(-1);
			  	}
				createDirectories(1, path_to_file, 0, comanda, sendbuf, sockfd);	
			}
		}

		if (strstr(comenzi, "-r") != NULL)
		{
			if ((strstr(path_to_file, "html") != NULL || strstr(path_to_file, "htm") != NULL) && (strstr(path_to_file, "http") == NULL) && (path_to_file[0] != '/'))
			{
				printf("%s\n", path_to_file);
				close(sockfd);
				/*	asignare socket-ul	*/							
			  	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
			  	{
			  		if (strstr(comenzi, "-o") != NULL)
						fprintf(flog, "%s", "Eroare la creare socket\n");
					else
						printf("Eroare la  creare socket.\n");
					exit(-1);
				} 
					/* conectare la server */
			 	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
			 	{
			 		if (strstr(comenzi, "-o") != NULL)
						fprintf(flog, "%s", "Eroare la conectare\n");
					else
			   			printf("Eroare la conectare\n");
				    exit(-1);
			  	}
		  		createDirectories(0, path_to_file, 0, comanda, sendbuf, sockfd);
			}
		}	
		memset(buffer, 0, 1000);
		memset(link, 0, 1000);
		memset(end_link, 0, 1000);
		memset(tag, 0, 1000);
		memset(path_to_file, 0 , 1000);
		memset(start_path, 0 , 1000);
		memset(end_path, 0 , 1000);
	}
	fclose(fp);
}

int main(int argc, char *argv[])
{
	int sockfd;
	int port = HTTP;
	char command[100];
	char server_ip[20];
	char *file_name = malloc(100*sizeof(char));
	char *aux = malloc(100*sizeof(char));
	char *init_path = malloc(100*sizeof(char));
	char sendbuf[MAXLEN];
	char recvbuf[MAXLEN];
	struct hostent *host_info = (struct hostent *) malloc(sizeof(struct hostent));

	if (argc < 2)
	{
		printf("Utilizare: ./send_msg adresa_pagina_server\n");
		exit(-1);
	}

	int i;

	for (i = 1; i < argc-1; i++)
	{
		if (strcmp(argv[i], "-o") == 0)
			flog = fopen(argv[i+1], "w");
		strcat(comenzi, argv[i]);
	}
		

	strcpy(aux, argv[argc-1]);
	aux = aux + strlen("http://");
	strcpy(file_name, strchr(aux, '/'));
	strncpy(server_name, aux, (strlen(aux) - strlen(file_name)));
	file_name = file_name + 1;

	host_info = gethostbyname(server_name);
	
	if (!host_info)
	{
		if (strstr(comenzi, "-o") != NULL)
			fprintf(flog, "%s", "Nu exista adresa IP asociata numelui\n");
		else
			printf("Nu exista adresa IP asociata numelui\n");
		exit(-1);
	}

	strcpy(server_ip, inet_ntoa( *((struct in_addr*)host_info->h_addr)));

  	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
  	{
  		if (strstr(comenzi, "-o") != NULL)
  			fprintf(flog, "%s", "Eroare la  creare socket.\n");
  		else
			printf("Eroare la  creare socket.\n");
		exit(-1);
	} 
	/* formarea adresei serverului */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	if (inet_aton(server_ip, &servaddr.sin_addr) <= 0 ) 
	{
  		if (strstr(comenzi, "-o") != NULL)
  			fprintf(flog, "%s", "Adresa IP invalida.\n");
  		else
	    	printf("Adresa IP invalida.\n");
	    exit(-1);
  	}
  	/* conectare la server */
 	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
 	{
		if (strstr(comenzi, "-o") != NULL)
  			fprintf(flog, "%s", "Eroare la conectare\n");
  		else	 
	    	printf("Eroare la conectare\n");
	    exit(-1);
  	}

  	char *search_file = malloc(100 * sizeof(char));
  	printf("%s\n", server_name);
  	printf("%s\n", file_name);
 	
 	int recurence = 0;

	createDirectories(0, file_name, 1, command, sendbuf, sockfd);

	fclose(flog);
  	close(sockfd);
  	return 0;
}