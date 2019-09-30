#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include "my_protocol.h"

#define SERVER_PORT 10007		/* arbitrary, but client and server must agree */
#define BUF_SIZE 4096			/* block transfer size */

typedef struct messagem{
	char *source;
	char *destiny;
	int message_type;
	char *content;
	char *timestamp;

}MESSAGE;

struct sockaddr_in init_client_info(char* name){
    struct hostent *h; /* info about server */
    h = gethostbyname(name);		/* look up host's IP address */
    if (!h) {
        perror("gethostbyname");
        exit(-1);
    }
    struct sockaddr_in channel;
    memset(&channel, 0, sizeof(channel));
    channel.sin_family= AF_INET;
    memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
    channel.sin_port= htons(SERVER_PORT);
    return channel;
}


int main(int argc, char **argv) 
{
    int c, s, bytes, maxs;
    char nickname[100] = {'\0'};
    char mensagem[BUF_SIZE] = {'\0'};
    char buf[BUF_SIZE];			/* buffer for incoming file */
    struct hostent *h;			/* info about server */
    struct sockaddr_in channel;		/* holds IP address */
    fd_set rset;
    
    if (argc != 3) {
        printf("Manual: client server-nome nickname\n");
        exit(-1);
    }

	channel= init_client_info(argv[1]); 
    
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        perror("socket");
        exit(-1);
    }
 
    c = connect(s, (struct sockaddr *) &channel, sizeof(channel));
    if (c < 0) {
        perror("connect");
        exit(-1);
    }else{	//antes de entrar no ciclo infinito, enviar mensagem no socket de registration com o user name
		sprintf(nickname, "%s", argv[2]);
		char type='0';
        char reg[BUF_SIZE];
            strcat(reg, "{\"type\":");
            size_t reg_len = strlen(reg);
            reg[reg_len]= type;
			
			strcat(reg, ",\"origin\":\"");
			strcat(reg, nickname);
			
			strcat(reg, "\",\"destination\":\"");
			strcat(reg, "null");
			
			strcat(reg, "\",\"message\":\"");
			
			strcat(reg, "null");
			strcat(reg, "\"}");
			write(s, reg, strlen(reg));
	}
    
    maxs=s;
    
    FD_ZERO(&rset);
    FD_SET(s, &rset);//watch for new connections
    FD_SET(0, &rset);//watch for something from the keyboard
    
    /* Socket is now set up and bound. Wait for connection and process it. */
    while (1) {
        
        FD_ZERO(&rset);
		FD_SET(s, &rset);//watch for new connections
		FD_SET(0, &rset);//watch for something from the keyboard
		
        int nready = select(maxs + 1, &rset, NULL, NULL, NULL);//blocks until something happen in the file descriptors watched
        if (FD_ISSET(s, &rset))//check if something happen in the listenfd (new connection)
        {
            int bytes = read(s, buf, BUF_SIZE);
            PROTOCOL* protocolo=parse_message(buf);
            if(bytes==0){
				printf("Server desconectado!\n");
				close(s);
				exit(0);
			}
			else{//else -> switchcase para cada tipo de mensagem
				
				PROTOCOL* protocolo=parse_message(buf);
				//passar da estrutura que já existe do protocolo para a nossa estrutura e adicionar timestamp
				MESSAGE *msg = (MESSAGE *) malloc(1 * sizeof(MESSAGE));
				msg->source = malloc(sizeof(char) * strlen(protocolo->origin));
				strcpy(msg->source, protocolo->origin);
				msg->destiny = malloc(sizeof(char) * strlen(protocolo->destination));
				strcpy(msg->destiny, protocolo->destination);
				msg->message_type = protocolo->type;
				msg->content = malloc(sizeof(char) * strlen(protocolo->message));
				strcpy(msg->content, protocolo->message);
				char timec[BUF_SIZE];
				time_t tt= time(NULL);
				struct tm* st = localtime(&tt);
				strftime(timec, 80, "%c", st);
				msg->timestamp = malloc(sizeof(char) * strlen(timec));
				strcpy(msg->timestamp, timec);
			
			switch(msg->message_type) {
				
							case 1:
								sprintf(mensagem, "%s(%s): %s\n", msg->source, msg->timestamp, msg->content);
								write(1, mensagem, strlen(mensagem));
								memset(buf,0,sizeof(buf));
								break;
								
							case 2:
								sprintf(mensagem, "%s(%s): %s\n", msg->source, msg->timestamp, msg->content);
								write(1, mensagem, strlen(mensagem));	
								memset(buf,0,sizeof(buf));
								break;
								
							case 3:
								sprintf(mensagem, "Utilizadores online: %s\n", msg->content);
								write(1, mensagem, strlen(mensagem));
								memset(buf,0,sizeof(buf));
								break;
								
							case 4:
								sprintf(mensagem, "Novo cliente: %s\n", msg->content);
								write(1, mensagem, strlen(mensagem));
								memset(buf,0,sizeof(buf));
								break;
								
							case 5:
								sprintf(mensagem, "Cliente desconectado: %s\n", msg->content);
								write(1, mensagem, strlen(mensagem));
								memset(buf,0,sizeof(buf));
								break;
                        
                        }
        }
	}
        if (FD_ISSET(0, &rset)) //check if something happen in the stdin (new input from the keyboard)
        {
            int bytes;
            char *dest;
            char *msg;
            char type;
            char destination[BUF_SIZE];
            memset(destination,0,sizeof(destination));
            char js[BUF_SIZE];
            memset(js,0,sizeof(js));
            memset(buf,0,sizeof(buf));
            strcat(js, "{\"type\":");
      
            scanf("%s", &type);
            
            size_t js_len = strlen(js);
			js[js_len]= type;
			
            switch(type){
				case '1':
					
					
					strcat(js, ",\"origin\":\"");
					strcat(js, nickname);
					
					strcat(js, "\",\"destination\":\"");
					dest= malloc(sizeof(char) * strlen(""));
					strcpy(dest, "");
					strcat(js, dest);
					
					strcat(js, "\",\"message\":\"");
					bytes = read(0, buf, BUF_SIZE);
					buf[bytes]='\0';
					msg= malloc(sizeof(char) * strlen(buf));
					strcpy(msg, buf);
					strtok(msg, "\n");
					strcat(js, msg);
					strcat(js, "\"}");
					free(msg);
					free(dest);
					break;
					
				case '2':
					
					strcat(js, ",\"origin\":\"");
					strcat(js, nickname);
					
					strcat(js, "\",\"destination\":\"");
					int bytesd= read(0, destination, BUF_SIZE);
					destination[bytesd]='\0';
					dest= malloc(sizeof(char) * strlen(destination));
					strcpy(dest, destination);
					strtok(dest, "\n");
					strcat(js, dest);
					
					strcat(js, "\",\"message\":\"");
					bytes = read(0, buf, BUF_SIZE);
					buf[bytes]='\0';
					msg= malloc(sizeof(char) * strlen(buf));
					strcpy(msg, buf);
					strtok(msg, "\n");
					strcat(js, msg);
					strcat(js, "\"}");
					free(msg);
					free(dest);
					break;
					
				case '3':
					
					strcat(js, ",\"origin\":\"");
					strcat(js, nickname);
					
					strcat(js, "\",\"destination\":\"");
					dest= malloc(sizeof(char) * strlen(""));
					strcpy(dest, "");
					strcat(js, dest);
					
					strcat(js, "\",\"message\":\"");
					msg= malloc(sizeof(char) * strlen(""));
					strcpy(msg, "");
					strcat(js, msg);
					strcat(js, "\"}");
					free(msg);
					free(dest);
					break;
				}	
			
			write(s, js, strlen(js));
        }
    }
}
