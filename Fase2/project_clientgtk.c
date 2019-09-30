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
#include <gtk/gtk.h>
#include "my_protocol.h"

#define SERVER_PORT 10007		/* arbitrary, but client and server must agree */
#define BUF_SIZE 5120			/* block transfer size */

GtkWidget *input;
GtkWidget *text_output;
GtkWidget *user_list;
GtkWidget *room_list;

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

void print_to_text_area(const gchar *text) {

	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text_output));

	mark = gtk_text_buffer_get_insert(buffer);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

//	/* Insert newline (only if there's already text in the buffer). */
	if (gtk_text_buffer_get_char_count(buffer)) {
		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
	}

	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
	gtk_text_buffer_insert(buffer, &iter, text, -1);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void print_to_list_user(const gchar *text) {

//	GtkTextBuffer *buffer;
//	GtkTextMark *mark;
//	GtkTextIter iter;
//
//	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (user_list));
//
//	mark = gtk_text_buffer_get_insert(buffer);
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//
//	/* Insert newline (only if there's already text in the buffer). */
//	if (gtk_text_buffer_get_char_count(buffer)) {
//		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
//	}
//
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//	gtk_text_buffer_insert(buffer, &iter, text, -1);
//	gtk_text_buffer_get_end_iter(buffer, &iter);
//	gtk_text_buffer_insert(buffer, 0, text, -1);
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	//user_list = gtk_text_view_new_with_buffer(buffer);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(user_list), buffer);

	//buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_text_buffer_set_text (buffer, text, -1);
}

void print_to_list_room(const gchar *text) {

	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (room_list));

	mark = gtk_text_buffer_get_insert(buffer);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

//	/* Insert newline (only if there's already text in the buffer). */
	if (gtk_text_buffer_get_char_count(buffer)) {
		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
	}

	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
	gtk_text_buffer_insert(buffer, &iter, text, -1);
}

// called when button is clicked
void on_send_button_clicked() {
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;
	char *text;

//	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text_output));
	text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
//
//	mark = gtk_text_buffer_get_insert(buffer);
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//
////	/* Insert newline (only if there's already text in the buffer). */
//	if (gtk_text_buffer_get_char_count(buffer)) {
//		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
//	}
//
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//	gtk_text_buffer_insert(buffer, &iter, text, -1);
//	gtk_entry_set_text(GTK_ENTRY(input), "");
	send_msg(0, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_send_room_clicked() {
	puts("on_send_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(1, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_list_user_clicked() {
	puts("on_list_user_clicked");
//	js = create_new_message("rooms", my_name, "null", "null");
	send_msg(3, 0, NULL);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_list_room_clicked(){
	puts("on_list_room_clicked");
	send_msg(7, 0, NULL);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_create_room_clicked(){
	puts("on_create_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(4, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_enter_room_clicked(){
	puts("on_enter_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(5, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}
void on_leave_room_clicked(){
	puts("on_leave_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(6, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_input_activate() {
	on_send_button_clicked();
}

// called when window is closed
void on_window_main_destroy() {
	gtk_main_quit();
	pthread_kill(tid, 2);
	exit(0);
}

int main(int argc, char **argv) 
{
	/*
	 * GUI
	 */
	GtkBuilder *builder;
	GtkWidget *window;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "glade/window_main.glade", NULL);

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
	gtk_builder_connect_signals(builder, NULL);

	// get pointers to the two labels
	input = GTK_WIDGET(gtk_builder_get_object(builder, "input"));
	text_output = GTK_WIDGET(gtk_builder_get_object(builder, "text_output"));
	user_list = GTK_WIDGET(gtk_builder_get_object(builder, "user_list"));
	room_list = GTK_WIDGET(gtk_builder_get_object(builder, "room_list"));

	g_object_unref(builder);
	gtk_widget_show(window);
	
	/*
	 * Client
	 */
	int flaglog=0;
    int c, s, bytes, maxs;
    char nickname[100] = {'\0'};
    char mensagem[BUF_SIZE];
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
		char destination[BUF_SIZE];
        memset(destination,0,sizeof(destination));
        char *dest;
		sprintf(nickname, "%s", argv[2]);
		int type=0;
        char reg[BUF_SIZE];
            strcat(reg, "{\"type\":");
            
            char t[BUF_SIZE];
            sprintf(t, "%d", type);
            strcat(reg, t);
			
			strcat(reg, ",\"origin\":\"");
			strcat(reg, nickname);
			
			strcat(reg, "\",\"destination\":\"");
			int bytesd= read(0, destination, BUF_SIZE);
			destination[bytesd]='\0';
			dest= malloc(sizeof(char) * strlen(destination));
			strcpy(dest, destination);
			strtok(dest, "\n");
			strcat(reg, dest);
			
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
		memset(mensagem,0,sizeof(mensagem));
        flaglog=1;
        FD_ZERO(&rset);
		FD_SET(s, &rset);//watch for new connections
		FD_SET(0, &rset);//watch for something from the keyboard
		
        int nready = select(maxs + 1, &rset, NULL, NULL, NULL);//blocks until something happen in the file descriptors watched
        if (FD_ISSET(s, &rset))//check if something happen in the listenfd (new connection)
        {
            int bytes = read(s, buf, BUF_SIZE);
            PROTOCOL* protocolo=parse_message(buf);
            if(bytes==0){
				printf("Server desconectou-se!\n");
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
								print_to_list_user(&buf[7]);
								break;
								
							case 2:
								sprintf(mensagem, "%s(%s): %s\n", msg->source, msg->timestamp, msg->content);
								write(1, mensagem, strlen(mensagem));	
								memset(buf,0,sizeof(buf));
								break;
								
							case 3:
								sprintf(mensagem, "Todos utilizadores online: %s\n", msg->content);
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
								
							case 8:
								sprintf(mensagem, "Saiu da sala. Volte a iniciar sessão para escolher outra sala.\n");
								write(1, mensagem, strlen(mensagem));
								memset(buf,0,sizeof(buf));
								break;
								
							case 9:
								sprintf(mensagem, "Salas disponíveis: %s\n", msg->content);
								write(1, mensagem, strlen(mensagem));
								memset(buf,0,sizeof(buf));
								break;
							
							case 10:
								sprintf(mensagem, "Utilizadores online: %s\n", msg->content);
								write(1, mensagem, strlen(mensagem));
								memset(buf,0,sizeof(buf));
								break;
								
							case 12:
								sprintf(mensagem, "%s\n", msg->content);
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
            int type;
            char destination[BUF_SIZE];
            memset(destination,0,sizeof(destination));
            char js[BUF_SIZE];
            memset(js,0,sizeof(js));
            memset(buf,0,sizeof(buf));
            strcat(js, "{\"type\":");
      
            scanf("%d", &type);
            
            size_t js_len = strlen(js);
            char t[20];
            sprintf(t, "%d", type);
            
            strcat(js, t);
			
            switch(type){
				case 1:
					
					
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
					
				case 2:
					
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
					
				case 3:
					
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
					
				case 6:
				
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
					
				case 7:
					
					strcat(js, ",\"origin\":\"");
					strcat(js, nickname);
					
					strcat(js, "\",\"destination\":\"");
					dest= malloc(sizeof(char) * strlen(""));
					strcpy(dest, "");
					strcat(js, dest);
					
					strcat(js, "\",\"message\":\"");
					bytesd= read(0, destination, BUF_SIZE);
					destination[bytesd]='\0';
					dest= malloc(sizeof(char) * strlen(destination));
					strcpy(dest, destination);
					strtok(dest, "\n");
					strcat(js, dest);
					strcat(js, "\"}");
					free(msg);
					free(dest);
					break;
					
				case 8:
				
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
					
				case 9:
					
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
					
				case 10:
					
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
					
				case 11:
					
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
				}	
			write(s, js, strlen(js));
        }
    }
}
