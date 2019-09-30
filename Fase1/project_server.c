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

#define SERVER_PORT 10007        /* arbitrary, but client and server must agree */
#define BUF_SIZE 4096            /* block transfer size */
#define QUEUE_SIZE 10

typedef struct messagem{
	char *source;
	char *destiny;
	int message_type;
	char *content;
	char *timestamp;

}MESSAGE;

typedef struct client {

    char *nickname;
    int sockID;
    int on;
    struct client *next;

} CLIENT;

struct sockaddr_in init_server_info() {
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);
    return servaddr;
}


int main(int argc, char *argv[]) 
{
    CLIENT *head = NULL;
    char mensagem[BUF_SIZE] = {'\0'};
    int s, maxs, b, l, bytes;
    fd_set rset;
    char buf[BUF_SIZE];            /* buffer for outgoing file */
    struct sockaddr_in servaddr;    /* hold's IP address */

    /* Passive open. Wait for connection. */
    servaddr = init_server_info();

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* create socket */
    if (s < 0) {
        perror("socket error");
        exit(-1);
    }

    b = bind(s, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (b < 0) {
        perror("bind error");
        exit(-1);
    }

    l = listen(s, QUEUE_SIZE);        /* specify queue size */
    if (l < 0) {
        perror("listen error");
        exit(-1);
    }

    FD_ZERO(&rset);
    FD_SET(s, &rset);//watch for new connections
    maxs = s;
    
    /* Socket is now set up and bound. Wait for connection and process it. */
    while (1) {

        FD_ZERO(&rset);
        FD_SET(s, &rset);

        if (head != NULL) {
            CLIENT *auxiliar = head;
            while (auxiliar != NULL) { 	//fazer fd_Set a todos os fd que queremos estar atentos
                if (auxiliar->on == 1) {
                    FD_SET(auxiliar->sockID, &rset);

                    if (maxs < auxiliar->sockID) {
                        maxs = auxiliar->sockID;
                    }

                }
                auxiliar = auxiliar->next;
            }
        }

        int nready = select(maxs + 1, &rset, NULL, NULL, NULL);//blocks until something happen in the file descriptors watched
        
        if (FD_ISSET(s, &rset)) {	//check if something happen in the listenfd (new connection)  //só para novas connecções
            socklen_t cliaddr_len = sizeof(servaddr);
            int connfd = accept(s, (struct sockaddr *) &servaddr, &cliaddr_len);
            printf("Nova conecção %d\n", connfd);
            
            //criar nó
            CLIENT *new = (CLIENT *) malloc(1 * sizeof(CLIENT));
            new->nickname = NULL;
            new->sockID = connfd;
            new->next = NULL;
            new->on = 1;
            if (head == NULL) {
                head = (CLIENT *) malloc(1 * sizeof(CLIENT));
                head = new;
                head->next=NULL;
            } else {
                CLIENT *auxhead = head;
                while (auxhead->next != NULL) {
                    auxhead = auxhead->next;
                }
                new->next=NULL;
                auxhead->next = new;
            }
			
            //atualizar maxs (se maxs for menor q o connfd, maxs é igual ao connfd)
            if (maxs < connfd) {
                maxs = connfd;
            }

        }//acaba o if
        else {    //else-> ciclo a percorrer todos os clientes-> fazer fd_isset a perguntar cada sockID de cada cliente se foi ele que mexeu
			char js[BUF_SIZE];
            memset(js,0,sizeof(js));
            size_t js_len;
            CLIENT *auxtest = head;
            while (auxtest != NULL) {
				
                if (FD_ISSET(auxtest->sockID, &rset)) {
                    int bytes = read(auxtest->sockID, buf, BUF_SIZE);
                    
                    if (bytes == 0) {
						//construir mensagem tipo 5
                        close(auxtest->sockID);
                        printf("Utilizador \"%s\" desconectado\n", auxtest->nickname);
                        auxtest->on = 0;

						strcat(js, "{\"type\":");
						size_t js_len = strlen(js);
						js[js_len]='5';
						strcat(js, ",\"origin\":\"");
						strcat(js, "");
						strcat(js, "\",\"destination\":\"");
						strcat(js, "");
						strcat(js, "\",\"message\":\"");
						strcat(js, auxtest->nickname);
						strcat(js, "\"}");

                        CLIENT *auxmsg = head;
                        while (auxmsg != NULL) {
                            write(auxmsg->sockID, js, strlen(js));
                            auxmsg = auxmsg->next;
                        }
                        
                    }else{
					//else -> switchcase para cada tipo de mensagem
					
						PROTOCOL* protocolo=parse_message(buf);
						
						switch(protocolo->type) {
							case 0:
							//construir mensagem tipo 4
								auxtest->on = 1;
								auxtest->nickname = malloc(sizeof(char) * strlen(protocolo->origin) + 1);
								strcpy(auxtest->nickname, protocolo->origin);
								printf("Novo cliente \"%s\"\n", auxtest->nickname);
								
								strcat(js, "{\"type\":");
								js_len = strlen(js);
								js[js_len]='4';
								strcat(js, ",\"origin\":\"");
								strcat(js, "");
								strcat(js, "\",\"destination\":\"");
								strcat(js, "");
								strcat(js, "\",\"message\":\"");
								strcat(js, auxtest->nickname);
								strcat(js, "\"}");
								
								CLIENT *auxmsg = head;
								while (auxmsg != NULL) {
									write(auxmsg->sockID, js, strlen(js));
									auxmsg = auxmsg->next;
								}
								memset(buf,0,sizeof(buf));
								break;
									
							case 1:
								auxmsg = head;
								while (auxmsg != NULL) {
									write(auxmsg->sockID, buf, strlen(buf));
									auxmsg = auxmsg->next;
								}
								memset(buf,0,sizeof(buf));
								break;
								
							case 2:
								auxmsg = head;
								while (auxmsg != NULL) {
									if(strcmp(auxmsg->nickname, protocolo->destination)==0 || strcmp(auxmsg->nickname, protocolo->origin)==0){
										write(auxmsg->sockID, buf, strlen(buf));
									
									}
									auxmsg = auxmsg->next;	
								}
								memset(buf,0,sizeof(buf));
								break;
								
							case 3:
								auxmsg = head;
								while (auxmsg != NULL) {
									if(auxmsg->on==1){
										strcat(mensagem, auxmsg->nickname);
										strcat(mensagem, ", ");
										
									
									}
									auxmsg = auxmsg->next;	
								}
								
								strcat(js, "{\"type\":");
								js_len = strlen(js);
								js[js_len]='3';
								strcat(js, ",\"origin\":\"");
								strcat(js, "");
								strcat(js, "\",\"destination\":\"");
								strcat(js, "");
								strcat(js, "\",\"message\":\"");
								strcat(js, mensagem);
								strcat(js, "\"}");
								write(auxtest->sockID, js, strlen(js));
								memset(buf,0,sizeof(buf));
								memset(mensagem,0,sizeof(mensagem));
								break;
                        
                        }
					}
				}
                
                auxtest = auxtest->next;
            }
		}
    }
    return 1;
}
