/*
	Path: 127.0.0.1:8888
*/

#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<pthread.h> //for threading , link with lpthread

#define BUFFER_SIZE_VIDEO 10485760

char buffer_video[BUFFER_SIZE_VIDEO];
/*{"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"ico", "image/ico"},
    {"zip", "image/zip"},
    {"gz", "image/gz"},
    {"tar", "image/tar"},
    {"htm", "text/html"},
    {"html", "text/html"},
    video/mp4
    */

//the thread function

void *connection_handler(void *);

int main(int argc , char *argv[]) {
	int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
		puts("Connection accepted");

		struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client;
		struct in_addr ipAddr = pV4Addr->sin_addr;
		char str[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
		printf("Client address: %s\n", str);

		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0) {
			perror("could not create thread");
			return 1;
		}
		
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned");
	}
	
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}
	
	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc) {
	//Get the socket descriptor 
	int sock = *(int*)socket_desc;
	char client_message[2000];

	int numRead = recv(sock , client_message , 2000 , 0);
    if (numRead < 1){
	    if (numRead == 0){
	        printf("The client was not read from: disconnected\n");
	    } else {
	        perror("The client was not read from");
	    }
	    close(sock);
    }
    printf("Aca %.*s\n", numRead, client_message); 

	//---------------------------------------
	FILE *fileptr;
	char *buffer;
	long filelen;

	//Aqui se obtiene lo que se pide en el request
	int largo_copia = 0;
	for(int i = 4; i < numRead; i++) {
		if(client_message[i] == ' ') {
			break;
		} else {
			largo_copia++;
			//printf("%c\n", client_message[i]);
		}
	}
	char salida[2000];

	//memcpy( salida, &client_message[4], largo_copia);
	strncpy(salida,client_message, largo_copia+4);
	//largo_copia++;
	salida[largo_copia+4] = '\0';

	printf("Salida es: %s el largo es: %ld\n", salida,strlen(salida));

	if( !strncmp(salida, "GET /favicon.ico", 16) ) {
		//icon_page.ico
		fileptr = fopen("icons/video-camera.ico", "rb");  // Open the file in binary mode
		char* fstr = "image/ico";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		printf("Opcion1 %ld / %s\n",filelen,salida);

		/* send file in 8KB block - last block may be smaller */
        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
            send(sock, salida, numRead, 0);
        }
	}else if ( !strncmp(salida, "GET /images/sharingan1.jpg", 26) ){
		//images/prueba.png
		fileptr = fopen("images/naruto.png", "rb");  // Open the file in binary mode
		char* fstr = "image/png";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		printf("Opcion3 %ld / %s\n",filelen,salida);
		char buffer_imagenes[10240];

		/* send file in 8KB block - last block may be smaller */
        while ((numRead = fread(buffer_imagenes, 1, 10240, fileptr)) > 0) {
        	//printf("Enviando...\n");
            send(sock, buffer_imagenes, numRead, 0);
        }
	} else if ( !strncmp(salida, "GET /videos/grito.mp4", 21) ){
		//images/prueba.png
		fileptr = fopen("videos/DBS_96.mp4", "rb");  // Open the file in binary mode
		char* fstr = "video/mp4";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		printf("Opcion3 %ld / %s\n",filelen,salida);
		
		/* send file in 8KB block - last block may be smaller */
        while ((numRead = fread(buffer_video, 1, sizeof(buffer_video), fileptr)) > 0) {
        	printf("Enviando...\n");
            int numSent = send(sock, buffer_video, numRead, 0);
            if (numSent <= 0){
	            if (numSent == 0){
	                printf("The client was not written to: disconnected\n");
	            } else {
	                perror("The client was not written to");
	            }
	            break;
	        }
        }
	}else if ( !strncmp(salida, "GET /", 5) ) {
		fileptr = fopen("index.html", "rb");  // Open the file in binary mode
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		printf("Opcion2 %ld / %s\n",filelen,salida);

		/* send file in 8KB block - last block may be smaller */
        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0)
        {
            send(sock, salida, numRead, 0);
        }
	}
		
	//Free the socket pointer
	free(socket_desc);
	close(sock); 
	
	return 0;
}
