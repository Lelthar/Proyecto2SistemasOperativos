/*
	Cliente terminal para el servidor de OSTube
	Proyecto 2 de Principios de Sistemas operativos
	Alumnos:
		Gerald Morales Alvarado - 2016042404
		Randall Delgado Miranda - 2016238520
	Profesor:
		Esteban Arias Mendez
*/
#include <stdio.h>
#include <string.h>	
#include <stdlib.h>	
#include <sys/socket.h>
#include <arpa/inet.h>	
#include <unistd.h>	
#include <pthread.h> 
#include <time.h>

#include "cliente.h"

#define IPSERVER "192.168.1.3"
#define LOCALHOST "127.0.0.1"
#define PUERTO 80
#define PETICION "GET /videos/avengers1.mp4 HTTP/1.1\nHost: 192.168.1.3\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:63.0) Gecko/20100101 Firefox/63.0\nAccept: video/webm,video/ogg,video/*;q=0.9,application/ogg;q=0.7,audio/*;q=0.6,*/*;q=0.5\nAccept-Language: en-US,en;q=0.5\nReferer: http://www.ostube.com/reproducir/avengers1.mp4\nRange: bytes=0-\nConnection: keep-alive\n"
#define BUFFER_SIZE_VIDEO 1048576
#define CANTIDADPETICIONES 100

pthread_mutex_t bytes_recibidos_mutex;
pthread_mutex_t tiempo_total_mutex;

double duracion_total;
long int bytes_recibidos;

int main(int argc , char *argv[]) {

	pthread_t thread_peticiones[CANTIDADPETICIONES];

	pthread_mutex_init(&bytes_recibidos_mutex, NULL);
	pthread_mutex_init(&tiempo_total_mutex, NULL);

	for(int i = 0; i < CANTIDADPETICIONES; i++) {
		
		if( pthread_create( &thread_peticiones[i] , NULL ,  peticiones_sockets , NULL) < 0) {

			printf("No se pudo crear el hilo\n");

			return 1;
		}
	
		
	}

	for(int i = 0; i < CANTIDADPETICIONES; i++) {
		pthread_join(thread_peticiones[i],NULL);
	}

	printf("Se terminó, se realizaron %d peticiones...\n",CANTIDADPETICIONES);
    double duracion_promedio = duracion_total / CANTIDADPETICIONES;
    printf("Duracion promedio de: %f segundos\n", duracion_promedio);
    printf("Cantidad de bytes recibidos: %ld bytes\n", bytes_recibidos);

	pthread_mutex_destroy(&bytes_recibidos_mutex);
	pthread_mutex_destroy(&tiempo_total_mutex);

	return 0;
}

void *peticiones_sockets(void *parametro) {

	clock_t inicio_consulta = clock();

	int sock;
	struct sockaddr_in server;
	char buffer[BUFFER_SIZE_VIDEO];

	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) {
		printf("Could not create socket");
		close(sock);
	}

	server.sin_addr.s_addr = inet_addr(IPSERVER);
	server.sin_family = AF_INET;
	server.sin_port = htons(PUERTO);

	//Conecta al servidor
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("connect failed. Error");
		close(sock);
	}

	if( send(sock , PETICION , strlen(PETICION) , 0) < 0) {
		puts("Send failed");
		close(sock);
	}

	int numRead = 0;
	long int total = 0;

	while(1) {
		memset(buffer ,0 , BUFFER_SIZE_VIDEO);
		numRead =  recv(sock , buffer , BUFFER_SIZE_VIDEO , 0);
		if(numRead == 0) {
			break;
		} else if(numRead < 0) {
			printf("Error\n");
		}else {
			total += numRead;
		}
	}

	pthread_mutex_lock(&bytes_recibidos_mutex); //Bloquea el recurso compartido

    bytes_recibidos += total;

    pthread_mutex_unlock(&bytes_recibidos_mutex); //Libera el recurso compartido

	clock_t final_consulta = clock();

    double time_spent = (double)(final_consulta - inicio_consulta) / CLOCKS_PER_SEC;

    pthread_mutex_lock(&tiempo_total_mutex); //Bloquea el recurso compartido

    duracion_total += time_spent;

    pthread_mutex_unlock(&tiempo_total_mutex); //Libera el recurso compartido

	close(sock);
}