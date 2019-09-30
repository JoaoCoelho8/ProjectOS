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
#include <errno.h>
#include <signal.h>

#define SERVER_PORT 10007        /* arbitrary, but client and server must agree */
#define BUF_SIZE 4096            /* block transfer size */
#define QUEUE_SIZE 10

typedef struct messagem {
    char *source;
    char *destiny;
    int message_type;
    char *content;
    char *timestamp;

} MESSAGE;

typedef struct client {

    char *nickname;
    int sockID;
    int on;
    struct client *next;

} CLIENT;

typedef struct chatroom {
    int id;
    int num_participantes;
    pthread_t thread_id;
    fd_set thset;
    struct client *pclients;
    struct chatroom *next;
} CHATROOM;

struct thread_data {
    int fdc;
    int chat_id;
};

struct sockaddr_in init_server_info() {
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);
    return servaddr;
}

CLIENT *head = NULL;
CHATROOM *room1 = NULL;

char mensagem[BUF_SIZE] = {'\0'};
int s, maxs, b, l, bytes;
/* buffer for outgoing file */
struct sockaddr_in servaddr;    /* hold's IP address */

void hdl(int sig) {
	
	//printf("Recebi sinal\n");

}

void *sendmessage(void *param) {
    
    sigset_t mask;
    sigset_t orig_mask;
    struct sigaction act;


    memset(&act, 0, sizeof(act));
    act.sa_handler = hdl;
    
    if (sigaction(SIGUSR1, &act, 0)) {
		perror ("sigaction");
	}

    sigemptyset (&mask);
	sigaddset (&mask, SIGUSR1);
 
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		perror ("sigprocmask");
	}

    //quando o thread gerente chega aqui, vai percorrer as salas até encontrar a dele e depois todo o código estará dentro do if onde a condição é se o thread id da sala é igual ao thread id do proprio thread
    char buf[BUF_SIZE];
    struct thread_data *my_data;
    my_data = (struct thread_data *) param;


    CHATROOM *auxroom = room1;
    CHATROOM *auxchat = NULL;

    while (auxroom != NULL) {

		int ptself= pthread_self();
		int ptid= auxroom->thread_id;
		
        if(ptself==ptid){
            break;
        }
        auxroom = auxroom->next;
    }
    
	int max_sd=0;
	fd_set rset;
	int nready;

    CLIENT *auxiliar = auxroom->pclients;
    while (auxiliar != NULL) {    //fazer fd_Set a todos os fd que queremos estar atentos
        if (auxiliar->on == 1) {
            FD_SET(auxiliar->sockID, &auxroom->thset);
            if (max_sd < auxiliar->sockID) {
                max_sd = auxiliar->sockID;
            }

        }
        auxiliar = auxiliar->next;
    }
	
    while (1) {
		
		FD_ZERO(&rset);
		rset = auxroom->thset;
        nready = pselect(max_sd + 1, &rset, NULL, NULL, NULL,&orig_mask);
		
		if (nready < 0 && errno == EINTR) {
			FD_ZERO(&auxroom->thset);
			if (auxroom->pclients != NULL) {
				CLIENT *auxiliar = auxroom->pclients;
				while (auxiliar != NULL) {    //fazer fd_Set a todos os fd que queremos estar atentos
				
					if (auxiliar->on == 1) {
						FD_SET(auxiliar->sockID, &auxroom->thset);
						if (max_sd < auxiliar->sockID) {
							max_sd = auxiliar->sockID;
						}
					}
					auxiliar = auxiliar->next;
				}
			}
        }
		
        char buf[BUF_SIZE];
        char mensagem[BUF_SIZE] = {'\0'};
        char js[BUF_SIZE];
        char t[20];
		int chatid;
		int flag=0;
        memset(js, 0, sizeof(js));
        memset(t, 0, sizeof(t));
        memset(buf,0,sizeof(buf));
        size_t js_len;
        CLIENT *auxtest = auxroom->pclients;
        CLIENT *auxadmin = auxroom->pclients;
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
                    js[js_len] = '5';
                    strcat(js, ",\"origin\":\"");
                    strcat(js, "");
                    strcat(js, "\",\"destination\":\"");
                    strcat(js, "");
                    strcat(js, "\",\"message\":\"");
                    strcat(js, auxtest->nickname);
                    strcat(js, "\"}");

                    CLIENT *auxmsg = auxroom->pclients;
                    while (auxmsg != NULL) {
                        write(auxmsg->sockID, js, strlen(js));
                        auxmsg = auxmsg->next;
                    }
                    pthread_kill(auxroom->thread_id, SIGUSR1);
                } else {
                    //else -> switchcase para cada tipo de mensagem
                    CLIENT *auxmsg = NULL;

                    PROTOCOL *protocolo = parse_message(buf);

                    switch (protocolo->type) {

                        case 1:
                            auxmsg = auxroom->pclients;
                            while (auxmsg != NULL) {
                                write(auxmsg->sockID, buf, strlen(buf));
                                auxmsg = auxmsg->next;
                            }
                            memset(buf, 0, sizeof(buf));
                            free(auxmsg);
                            break;

                        case 2:
                            auxmsg = auxroom->pclients;
                            while (auxmsg != NULL) {
                                if (strcmp(auxmsg->nickname, protocolo->destination) == 0 ||
                                    strcmp(auxmsg->nickname, protocolo->origin) == 0) {
                                    write(auxmsg->sockID, buf, strlen(buf));

                                }
                                auxmsg = auxmsg->next;
                            }
                            memset(buf, 0, sizeof(buf));
                            free(auxmsg);
                            break;

                        case 3:
							auxchat= room1;
							while (auxchat != NULL) {
								
								auxmsg = auxchat->pclients;
								while (auxmsg != NULL) {
									if (auxmsg->on == 1) {
										strcat(mensagem, auxmsg->nickname);
										strcat(mensagem, ", ");


									}
									auxmsg = auxmsg->next;
								}
								
								auxchat = auxchat->next;
							}
							
							strcat(js, "{\"type\":");
							js_len = strlen(js);
							js[js_len] = '3';
							strcat(js, ",\"origin\":\"");
							strcat(js, "");
							strcat(js, "\",\"destination\":\"");
							strcat(js, "");
							strcat(js, "\",\"message\":\"");
							strcat(js, mensagem);
							strcat(js, "\"}");
							write(auxtest->sockID, js, strlen(js));
							memset(buf, 0, sizeof(buf));
							memset(mensagem, 0, sizeof(mensagem));
                            break;

                        case 6:
							auxmsg = auxroom->pclients;
							
							//auxroom = room1;
							CHATROOM *room = (CHATROOM *) malloc(1 * sizeof(CHATROOM));
							int counter=1;
							CHATROOM *auxroom2 = room1;
							while (auxroom2 != NULL) {
								if(auxroom2->id==counter){
									counter ++;
								}else{
									
								}
								auxroom2 = auxroom2->next;
							}
							memset(t, 0, sizeof(t));
							
							
							room->id = counter;
							room->num_participantes = 1;
							
							//introduzir na lista de salas
							auxroom2 = room1;
							if (room1 == NULL) {
								room1 = (CHATROOM *) malloc(1 * sizeof(CHATROOM));
								room1 = room;
								room1->next = NULL;
							} else {
								auxroom2 = room1;
								while (auxroom2->next != NULL) {
									auxroom2 = auxroom2->next;
								}
								room->next = NULL;
								auxroom2->next = room;
							}
							
							
							CLIENT *auxcliente= auxroom->pclients;
							CLIENT *auxcliente2= NULL;
							
							if(auxmsg->sockID==auxtest->sockID){
								
								if(auxmsg->next==NULL){
									auxroom->pclients=NULL;
									auxroom->num_participantes=0;
								}
								else{
									auxroom->pclients=auxroom->pclients->next;
									auxroom->num_participantes--;
								}
							}
							else{
								while(auxcliente != NULL){
									if(auxcliente->sockID==auxtest->sockID){
										if(auxcliente->next==NULL){
											auxcliente2->next=NULL;
											auxroom->num_participantes--;
										}else{
											auxcliente2->next=auxcliente->next;
											auxroom->num_participantes--;
										}
									}
									auxcliente2=auxcliente;
									auxcliente=auxcliente->next;
								}
							}
							
							
							struct thread_data data;
							data.chat_id = counter;
							data.fdc = my_data->fdc;
							room->pclients = auxtest;
							room->pclients->next = NULL;
							pthread_kill(auxroom->thread_id, SIGUSR1);
							pthread_create(&room->thread_id, NULL, sendmessage, &data);
							
							//type 12 Nova sala x
							strcat(js, "{\"type\":");
							int type=12;
							sprintf(t, "%d", type);
                            strcat(js, t);
							strcat(js, ",\"origin\":\"");
							strcat(js, "");
							strcat(js, "\",\"destination\":\"");
							strcat(js, "");
							strcat(js, "\",\"message\":\"");
							strcat(js, "Sala ");
							char cnt[10];
							sprintf(cnt, "%d", counter);
							strcat(js, cnt);
							strcat(js, " criada");
							strcat(js, "\"}");
                            write(auxtest->sockID, js, strlen(js));
                            break;

                        case 7:
							
							memset(t, 0, sizeof(t));
							//introduzir na nova sala
							chatid = atoi(protocolo->message);
							auxroom2 = room1;
							CLIENT *auxiliar= auxtest;
							while (auxroom2 != NULL) {
								if(auxroom2->id==chatid){
									if(auxroom2->num_participantes<5){
										CLIENT *auxclient = auxroom2->pclients;
										if(auxclient == NULL){
											auxclient=auxiliar;
											auxclient->next=NULL;
										}else {
											while (auxclient->next != NULL) {
												auxclient = auxclient->next;
											}
											auxiliar->next=NULL;
											auxclient->next = auxiliar;
										}
										auxroom2->num_participantes++;
										
										
										//remover da sala
										auxmsg = auxroom->pclients;
										auxcliente= auxroom->pclients;
										auxcliente2= NULL;
										
										if(auxmsg->sockID==auxtest->sockID){
											if(auxmsg->next==NULL){
												auxroom->pclients=NULL;
												auxroom->num_participantes=0;
											}
											else{
												auxroom->pclients=auxroom->pclients->next;
												auxroom->num_participantes--;
											}
										}
										else{
											while(auxcliente != NULL){
												if(auxcliente->sockID==auxtest->sockID){
													if(auxcliente->next==NULL){
														auxcliente2->next=NULL;
														auxroom->num_participantes--;
													}else{
														auxcliente2->next=auxcliente->next;
														auxroom->num_participantes--;
													}
												}
												auxcliente2=auxcliente;
												auxcliente=auxcliente->next;
											}
										}
										
										strcat(js, "{\"type\":");
										int type=12;
										sprintf(t, "%d", type);
										strcat(js, t);
										strcat(js, ",\"origin\":\"");
										strcat(js, "");
										strcat(js, "\",\"destination\":\"");
										strcat(js, "");
										strcat(js, "\",\"message\":\"");
										strcat(js, "Mudança bem sucedida.");
										strcat(js, "\"}");
										write(auxtest->sockID, js, strlen(js));
										
										pthread_kill(auxroom->thread_id, SIGUSR1);
										pthread_kill(auxroom2->thread_id, SIGUSR1);
										break;
									}
									else{
										//type 12 cheio
										strcat(js, "{\"type\":");
										int type=12;
										sprintf(t, "%d", type);
										strcat(js, t);
										strcat(js, ",\"origin\":\"");
										strcat(js, "");
										strcat(js, "\",\"destination\":\"");
										strcat(js, "");
										strcat(js, "\",\"message\":\"");
										strcat(js, "Sala cheia!");
										strcat(js, "\"}");
										write(auxtest->sockID, js, strlen(js));
										close(auxtest->sockID);
										pthread_kill(auxroom->thread_id, SIGUSR1);
									}
								}
								auxroom2 = auxroom2->next;
							}
							if(auxroom2==NULL){
								//type 12
								strcat(js, "{\"type\":");
								int type=12;
								sprintf(t, "%d", type);
								strcat(js, t);
								strcat(js, ",\"origin\":\"");
								strcat(js, "");
								strcat(js, "\",\"destination\":\"");
								strcat(js, "");
								strcat(js, "\",\"message\":\"");
								strcat(js, "Sala não encontrada!");
								strcat(js, "\"}");
								write(auxtest->sockID, js, strlen(js));
								close(auxtest->sockID);
								//pthread_kill(auxroom->thread_id, SIGUSR1);
							}
							free(auxmsg);
                            break;

                        case 8:
							auxmsg = auxroom->pclients;
							auxcliente= auxroom->pclients;
							auxcliente2= NULL;
							
							if(auxmsg->sockID==auxtest->sockID){
								
								if(auxmsg->next==NULL){
									auxroom->pclients=NULL;
									auxroom->num_participantes=0;
								}
								else{
									auxroom->pclients=auxroom->pclients->next;
									auxroom->num_participantes--;
								}
							}
							else{
								while(auxcliente != NULL){
									if(auxcliente->sockID==auxtest->sockID){
										if(auxcliente->next==NULL){
											auxcliente2->next=NULL;
											auxroom->num_participantes--;
										}else{
											auxcliente2->next=auxcliente->next;
											auxroom->num_participantes--;
										}
									}
									auxcliente2=auxcliente;
									auxcliente=auxcliente->next;
								}
							}
							printf("Utilizador \"%s\" saiu da sala %d\n", auxtest->nickname, auxroom->id);
							auxtest->on = 0;

							strcat(js, "{\"type\":");
							size_t js_len = strlen(js);
							js[js_len] = '5';
							strcat(js, ",\"origin\":\"");
							strcat(js, "");
							strcat(js, "\",\"destination\":\"");
							strcat(js, "");
							strcat(js, "\",\"message\":\"");
							strcat(js, auxtest->nickname);
							strcat(js, "\"}");

							auxmsg = auxroom->pclients;
							while (auxmsg != NULL) {
								write(auxmsg->sockID, js, strlen(js));
								auxmsg = auxmsg->next;
							}
							
							//type 8 voltar a iniciar sessão e escolher outra sala
							strcat(js, "{\"type\":");
							type=8;
							sprintf(t, "%d", type);
                            strcat(js, t);
							strcat(js, ",\"origin\":\"");
							strcat(js, "");
							strcat(js, "\",\"destination\":\"");
							strcat(js, "");
							strcat(js, "\",\"message\":\"");
							char frase[BUF_SIZE]= "Saiu da sala.";
							char *frasem= malloc(sizeof(char) * strlen(frase));
							strcpy(frasem, frase);
							strcat(js, frasem);
							strcat(js, "\"}");
                            write(auxtest->sockID, js, strlen(js));
							//pthread_kill(auxroom->thread_id, SIGUSR1);
							close(auxtest->sockID);
                            break;

                        case 9:
                            auxchat = room1;
                            char msgaux[BUF_SIZE] = {'\0'};
                            while (auxchat != NULL) {

                                sprintf(msgaux, "%d, ", auxchat->id);
                                strcat(mensagem, msgaux);
                                auxchat = auxchat->next;
                            }

                            strcat(js, "{\"type\":");
                            js_len = strlen(js);
                            js[js_len] = '9';
                            strcat(js, ",\"origin\":\"");
                            strcat(js, "");
                            strcat(js, "\",\"destination\":\"");
                            strcat(js, "");
                            strcat(js, "\",\"message\":\"");
                            strcat(js, mensagem);
                            strcat(js, "\"}");
                            write(auxtest->sockID, js, strlen(js));
                            memset(buf, 0, sizeof(buf));
                            memset(mensagem, 0, sizeof(mensagem));
                            free(auxmsg);
                            break;

                        case 10:
							memset(t, 0, sizeof(t));
							auxmsg = auxroom->pclients;
                            while (auxmsg != NULL) {
                                if (auxmsg->on == 1) {
                                    strcat(mensagem, auxmsg->nickname);
                                    strcat(mensagem, ", ");


                                }
                                auxmsg = auxmsg->next;
                            }

                            strcat(js, "{\"type\":");
                            
                            type=10;
                            
							sprintf(t, "%d", type);
                            strcat(js, t);
                            
                            strcat(js, ",\"origin\":\"");
                            strcat(js, "");
                            strcat(js, "\",\"destination\":\"");
                            strcat(js, "");
                            strcat(js, "\",\"message\":\"");
                            strcat(js, mensagem);
                            strcat(js, "\"}");
                            write(auxtest->sockID, js, strlen(js));
                            memset(buf, 0, sizeof(buf));
                            memset(mensagem, 0, sizeof(mensagem));
                            free(auxmsg);
                            break;
                            
                        case 11:
							auxadmin = auxroom->pclients;
							if(auxadmin->sockID==auxtest->sockID){
								
								//tirar da sala e mensagem de bem sucedido
								auxmsg = auxroom->pclients;
								auxcliente= auxroom->pclients;
								auxcliente2= NULL;
								
								while(auxcliente != NULL){
									if(strcmp(auxcliente->nickname, protocolo->message)==0){
										if(auxcliente->next==NULL){
											auxcliente2->next=NULL;
											auxroom->num_participantes--;
										}else{
											auxcliente2->next=auxcliente->next;
											auxroom->num_participantes--;
										}
										strcat(js, "{\"type\":");
										int type=12;
										sprintf(t, "%d", type);
										strcat(js, t);
										strcat(js, ",\"origin\":\"");
										strcat(js, "");
										strcat(js, "\",\"destination\":\"");
										strcat(js, "");
										strcat(js, "\",\"message\":\"");
										strcat(js, "Ban bem sucedida");
										strcat(js, "\"}");
										write(auxtest->sockID, js, strlen(js));
										close(auxcliente->sockID);
										pthread_kill(auxroom->thread_id, SIGUSR1);
										free(auxmsg);
										break;
									}
									auxcliente2=auxcliente;
									auxcliente=auxcliente->next;
								}
								
								if(auxcliente==NULL) {
									//mensagem nao encontrado
									strcat(js, "{\"type\":");
									int type=12;
									sprintf(t, "%d", type);
									strcat(js, t);
									strcat(js, ",\"origin\":\"");
									strcat(js, "");
									strcat(js, "\",\"destination\":\"");
									strcat(js, "");
									strcat(js, "\",\"message\":\"");
									strcat(js, "Cliente ñ encontrado!");
									strcat(js, "\"}");
									write(auxtest->sockID, js, strlen(js));
									break;
								}
								
							}else{
								write(1, "elsefora", strlen("elsefora"));
								strcat(js, "{\"type\":");
								int type=12;
								sprintf(t, "%d", type);
								strcat(js, t);
								strcat(js, ",\"origin\":\"");
								strcat(js, "");
								strcat(js, "\",\"destination\":\"");
								strcat(js, "");
								strcat(js, "\",\"message\":\"");
								strcat(js, "Não tem permissão!");
								strcat(js, "\"}");
								write(auxtest->sockID, js, strlen(js));
								break;
							}
						break;
                    }
                }
            }

            auxtest = auxtest->next;
        }

    }


}


void *createclient(void *param) {

    char buf[BUF_SIZE];
    char js[BUF_SIZE];
    char t[20];

    struct thread_data *my_data;
    my_data = (struct thread_data *) param;

    printf("Nova conecção %d\n", my_data->fdc);


    int bytes = read(my_data->fdc, buf, BUF_SIZE);


    PROTOCOL *protocolo = parse_message(buf);

   // if (protocolo->type == 0) {
		CLIENT *new = (CLIENT *) malloc(1 * sizeof(CLIENT));
        new->on = 1;
        new->nickname = malloc(sizeof(char) * strlen(protocolo->origin) + 1);

        strcpy(new->nickname, protocolo->origin);

        printf("Novo cliente \"%s\"\n", new->nickname);
        new->sockID = my_data->fdc;
        new->next = NULL;


        //no destination da mensagem está o id da sala para onde vai o cliente
       
        int chatid = atoi(protocolo->destination);

        CHATROOM *auxhead = room1;
        while (auxhead != NULL) {
			if (auxhead->id == chatid) {
				break;
			}
			auxhead=auxhead->next;
		}
            if (auxhead!=NULL) {
				
				if(auxhead->num_participantes<5) {
					CLIENT *auxclient = auxhead->pclients;
					if(auxclient == NULL){
						auxclient=new;
					}else {
						while (auxclient->next != NULL) {
							auxclient = auxclient->next;
						}
						auxclient->next = new;
					}
					
					auxhead->num_participantes++;

					//construir mensagem tipo 4

					strcat(js, "{\"type\":");
					size_t js_len = strlen(js);
					js[js_len] = '4';
					strcat(js, ",\"origin\":\"");
					strcat(js, "");
					strcat(js, "\",\"destination\":\"");
					strcat(js, "");
					strcat(js, "\",\"message\":\"");
					strcat(js, new->nickname);
					strcat(js, "\"}");

					CLIENT *auxmsg = auxhead->pclients;
					while (auxmsg != NULL) {
						write(auxmsg->sockID, js, strlen(js));
						auxmsg = auxmsg->next;
					}
					
					
					memset(buf, 0, sizeof(buf));
					
					//sinal
					pthread_kill(auxhead->thread_id, SIGUSR1);
				}
				else{
					//type 12 cheio
					strcat(js, "{\"type\":");
					int type=12;
					sprintf(t, "%d", type);
					strcat(js, t);
					strcat(js, ",\"origin\":\"");
					strcat(js, "");
					strcat(js, "\",\"destination\":\"");
					strcat(js, "");
					strcat(js, "\",\"message\":\"");
					strcat(js, "Sala cheia!");
					strcat(js, "\"}");
					write(new->sockID, js, strlen(js));
					close(new->sockID);
				}
            }

        else{
            CHATROOM *room = (CHATROOM *) malloc(1 * sizeof(CHATROOM));
            room->id = chatid;
            room->num_participantes = 1;
            room->pclients = new;
            new->next= NULL;

            //introduzir na lista de salas

            CHATROOM *auxroom = room1;

            if (room1 == NULL) {
                room1 = (CHATROOM *) malloc(1 * sizeof(CHATROOM));
                room1 = room;
                room1->next = NULL;
            } else {
                auxroom = room1;
                while (auxroom->next != NULL) {
                    auxroom = auxroom->next;
                }
                room->next = NULL;
                auxroom->next = room;
            }


            struct thread_data data;
            data.chat_id = chatid;
            data.fdc = my_data->fdc;

            pthread_create(&room->thread_id, NULL, sendmessage, &data);
        }
    //}
    pthread_exit(1);
}

struct thread_data thdata[10];

int main(int argc, char *argv[]) {

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
    
    maxs = s;

    /* Socket is now set up and bound. Wait for connection and process it. */
    int i = 0;
    while (1) {

        socklen_t cliaddr_len = sizeof(servaddr);
        int connfd = accept(s, (struct sockaddr *) &servaddr, &cliaddr_len);

        thdata[i].fdc = connfd;

        pthread_t rececionista;
        pthread_create(&rececionista, NULL, createclient, &thdata[i]);

        i++;
    }
}
