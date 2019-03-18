/*
	Servidor OSTube 
	Proyecto 2 de Principios de Sistemas operativos
	Alumnos:
		Gerald Morales Alvarado - 2016042404
		Randall Delgado Miranda - 2016238520
	Profesor:
		Esteban Arias Mendez
*/

#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>	
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
 
#include "vector.h"
#include "server.h"

#define BUFFER_SIZE_INDEX 60000
#define BUFFER_SIZE_VIDEO 1048576
#define BUFFER_SIZE_IMAGES 10240
#define PARTEALTASLIDESHOW "partes_html/parteAltaIndexSlideshow"
#define PARTEBAJASLIDESHOW "partes_html/parteBajaIndexSlideshow"
#define PARTEALTAVIDEO "partes_html/parteAltaVideos"
#define PARTEBAJAVIDEO "partes_html/parteBajaVideos"
#define PARTEALTAINDEX "partes_html/parteAltaIndex"
#define PARTEBAJAINDEX "partes_html/parteBajaIndex"
#define IPSERVER "192.168.1.3"
#define LOCALHOST "127.0.0.1"
#define PUERTO 80


char pagina_index[BUFFER_SIZE_INDEX];
char pagina_index_slice[BUFFER_SIZE_INDEX];

char pagina_index_marvel[BUFFER_SIZE_INDEX];
char pagina_index_slice_marvel[BUFFER_SIZE_INDEX];

char pagina_index_dc[BUFFER_SIZE_INDEX];
char pagina_index_slice_dc[BUFFER_SIZE_INDEX];

char buffer_video[BUFFER_SIZE_VIDEO];

vector lista_videos;
vector lista_imagenes;
vector lista_descripciones;

vector lista_marvel_videos;
vector lista_marvel_imagenes;
vector lista_marvel_descripciones;

vector lista_dc_videos;
vector lista_dc_imagenes;
vector lista_dc_descripciones;

vector lista_descripciones_generar;
vector lista_videos_generar;
vector lista_imagenes_generar;
vector lista_partes_html;


vector lista_ip_usuarios;

struct stat file_datos; //datos de un archivo

/*
Informacion del servidor
*/
time_t tiempo_actual;
struct tm tiempo_inicio; //Tiempo de inicio del  servidor
long cantidad_bytes_transferidos; //Cantidad de bytes tranferidos
double velocidad_total; //Velocidad promedio de transferencia
int cantidad_ip_distintas; //Cantidad de ips distintas
int cantidad_solicitudes; //Cantidad de solicitudes realizadas
int cantidad_threads_creados; //Cantidad de threads que se crearon

pthread_mutex_t bytes_transferidos_mutex;
pthread_mutex_t velocidad_total_mutex;
pthread_mutex_t ip_distintas_mutex;
pthread_mutex_t cantidad_solicitudes_mutex;
pthread_mutex_t threads_creados_mutex;
pthread_mutex_t log_mutex;
pthread_mutex_t actualizar_mutex;

int main(int argc , char *argv[]) {

    signal(SIGPIPE,SIG_IGN);

	int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		escribirBitacora("Error: No se pudo crear el socket");
	}

	//Carga los datos de los directorios

    vector_init(&lista_ip_usuarios);

    /* Inicialización de los mutex */
    pthread_mutex_init(&bytes_transferidos_mutex, NULL);
    pthread_mutex_init(&velocidad_total_mutex, NULL);
    pthread_mutex_init(&ip_distintas_mutex, NULL);
    pthread_mutex_init(&cantidad_solicitudes_mutex, NULL);
    pthread_mutex_init(&threads_creados_mutex, NULL);
    pthread_mutex_init(&log_mutex, NULL);
    pthread_mutex_init(&actualizar_mutex, NULL);
    
    vector_init(&lista_videos_generar);
	vector_init(&lista_imagenes_generar);
	vector_init(&lista_descripciones_generar);
	vector_init(&lista_partes_html);

    cargar_archivos_vectores();
    revisar_generacion_index("datos_revision.md5");
    tiempo_actual = time(NULL);
   	tiempo_inicio = *localtime(&tiempo_actual);

   	cantidad_bytes_transferidos = 0; //Cantidad de bytes tranferidos
	velocidad_total = 0; //Velocidad promedio de transferencia
	cantidad_ip_distintas = 0; //Cantidad de ips distintas
	cantidad_solicitudes = 0; //Cantidad de solicitudes realizadas
	cantidad_threads_creados = 0; //Cantidad de threads que se crearon

   	pthread_t *hilo_menu = (pthread_t *)malloc(sizeof(pthread_t));

   	pthread_t *hilo_actualizador_index = (pthread_t *)malloc(sizeof(pthread_t));
   	int parametro = 0;

    pthread_create(hilo_menu, NULL, menu_servidor,&parametro);
    int parametro_index = 0;

    pthread_create(hilo_actualizador_index, NULL, actualizar_index,&parametro_index);
	server.sin_family = AF_INET;
	//server.sin_addr.s_addr = inet_addr(IPSERVER); //Ip del servidor
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PUERTO); //Puerto del servidor
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		escribirBitacora("Error: Fallo del bind");
		return 1;
	}

	//Listen
	listen(socket_desc , 3);

	escribirBitacora("Inicio el servidor...");
	
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {

		struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client;
		struct in_addr ipAddr = pV4Addr->sin_addr;
		char *str_ip = malloc(INET_ADDRSTRLEN);
		inet_ntop( AF_INET, &ipAddr, str_ip, INET_ADDRSTRLEN );

		time_t tiempo_conexion;
		struct tm tiempo_inicio_conexion; //Tiempo de inicio del  servidor

		tiempo_conexion = time(NULL);
   		tiempo_inicio_conexion = *localtime(&tiempo_conexion);

   		char datos_fecha_conexion[200];
   		memset(datos_fecha_conexion,0,200);

   		sprintf(datos_fecha_conexion,"Hora de inicio: %d-%d-%d %d:%d:%d", tiempo_inicio_conexion.tm_mday, tiempo_inicio_conexion.tm_mon + 1, tiempo_inicio_conexion.tm_year + 1900, tiempo_inicio_conexion.tm_hour, tiempo_inicio_conexion.tm_min, tiempo_inicio_conexion.tm_sec);

		char peticion_realizada[200];
		memset(peticion_realizada,0,200);
		strcat(peticion_realizada,"Se conectó el usuario ");
		strcat(peticion_realizada,str_ip);
		strcat(peticion_realizada," - ");
		strcat(peticion_realizada,datos_fecha_conexion);

		escribirBitacora(peticion_realizada);

		if( !contains(&lista_ip_usuarios,str_ip)) {
			vector_add(&lista_ip_usuarios, str_ip);

			pthread_mutex_lock(&ip_distintas_mutex); //Bloquea el recurso compartido

		    cantidad_ip_distintas += 1;

		    pthread_mutex_unlock(&ip_distintas_mutex); //Libera el recurso compartido
		}

		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0) {

			escribirBitacora("Error: No se pudo crear el hilo");

			return 1;
		}

		pthread_mutex_lock(&threads_creados_mutex); //Bloquea el recurso compartido

	    cantidad_threads_creados += 1;

	    pthread_mutex_unlock(&threads_creados_mutex); //Libera el recurso compartido
	
	}

	pthread_mutex_destroy(&bytes_transferidos_mutex);
	pthread_mutex_destroy(&velocidad_total_mutex);
	pthread_mutex_destroy(&ip_distintas_mutex);
	pthread_mutex_destroy(&cantidad_solicitudes_mutex);
	pthread_mutex_destroy(&threads_creados_mutex);
	pthread_mutex_destroy(&log_mutex);

	if (client_sock < 0) {

		escribirBitacora("Error: Fallo la creación del socket");

		return 1;
	}
	
	return 0;
}

/*
 * Hilo para realizar la petición del cliente
 * */
void *connection_handler(void *socket_desc) {
	
	clock_t inicio_consulta = clock();

	int sock = *(int*)socket_desc;
	char client_message[2000];

	int numRead = recv(sock , client_message , 2000 , 0);
    if (numRead < 1){
	    if (numRead == 0){
	    	escribirBitacora("Error: El cliente no está leyendo, se desconectó.\n");
	    } else {
	    	escribirBitacora("Error: El cliente no está leyendo.\n");
	        //perror("The client was not read from");
	    }
	    close(sock);
    }
    //printf("\n%s\n",client_message );
    //printf("Aca %.*s\n", numRead, client_message);
    pthread_mutex_lock(&cantidad_solicitudes_mutex); //Bloquea el recurso compartido

    cantidad_solicitudes += 1;

    pthread_mutex_unlock(&cantidad_solicitudes_mutex); //Libera el recurso compartido

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
		}
	}
	char salida[2000];

	strncpy(salida,client_message, largo_copia+4);

	salida[largo_copia+4] = '\0';

	if( !strncmp(salida, "GET /favicon.ico\n", 16) ) {
		escribirBitacora("Petición get de /favicon.ico");
		fileptr = fopen("icons/video-camera.ico", "rb");  // Open the file in binary mode
		char* fstr = "image/ico";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

        cantidad_bytes_transferidos += strlen(salida);

        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
            send(sock, salida, numRead, 0);
            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

            cantidad_bytes_transferidos += numRead;

            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
        }

        fclose(fileptr);
	}else if ( !strncmp(salida, "GET /images/", 12) ){
		//images/prueba.png
		char path_imagen[2000];
		strncpy(path_imagen,salida+5, strlen(salida));
		//printf("Error1: %s\n",path_imagen);
		if( access( path_imagen, F_OK ) != -1 ) {
			char peticion_realizada[80];
			memset(peticion_realizada,0,80);
			strcat(peticion_realizada,"Peticion get de /images/");
			strcat(peticion_realizada,path_imagen);

			escribirBitacora(peticion_realizada);

			//free(peticion_realizada);

			fileptr = fopen(path_imagen, "rb");  // Open the file in binary mode
			char* fstr = "image/png";
			sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
			send(sock, salida, strlen(salida), 0);

			pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	        cantidad_bytes_transferidos += strlen(salida);

	        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

			char buffer_imagenes[BUFFER_SIZE_IMAGES];

	        while ((numRead = fread(buffer_imagenes, 1, BUFFER_SIZE_IMAGES, fileptr)) > 0) {
	       
	            send(sock, buffer_imagenes, numRead, 0);

	            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	            cantidad_bytes_transferidos += numRead;

	            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
	        }
	        fclose(fileptr);
		} else {
			char peticion_realizada[80];
			memset(peticion_realizada,0,80);
			strcat(peticion_realizada,"Error de peticion get de /images/");
			strcat(peticion_realizada,path_imagen);

			escribirBitacora(peticion_realizada);

			//free(peticion_realizada);

			fileptr = fopen("archivos_html/error_index.html", "rb");  // Open the file in binary mode
			char* fstr = "text/html";
			sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
			send(sock, salida, strlen(salida), 0);

			pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	        cantidad_bytes_transferidos += strlen(salida);

	        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

	        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
	            send(sock, salida, numRead, 0);

	            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	            cantidad_bytes_transferidos += numRead;

	            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
	        }
	        fclose(fileptr);
		}

		
	} else if ( !strncmp(salida, "GET /videos/", 12) ){
		char path_video[2000];
		strncpy(path_video,salida+5, strlen(salida));

		if( access( path_video, F_OK ) != -1 ) {
			char peticion_realizada[80];
			memset(peticion_realizada,0,80);
			strcat(peticion_realizada,"Petición get de /videos/");
			strcat(peticion_realizada,path_video);

			escribirBitacora(peticion_realizada);

		    fileptr = fopen(path_video, "rb");  // Open the file in binary mode
			char* fstr = "video/mp4";
			sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
			send(sock, salida, strlen(salida), 0);

			pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	        cantidad_bytes_transferidos += strlen(salida);

	        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

	        while ((numRead = fread(buffer_video, 1, BUFFER_SIZE_VIDEO, fileptr)) > 0) {
	            int numSent = send(sock, buffer_video, numRead, 0);
	            if (numSent <= 0){
		            if (numSent == 0){
		                //printf("The client was not written to: disconnected\n");
		                escribirBitacora("Error: El cliente no está conectado.");
		            } else {
		                //perror("The client was not written to");
		                escribirBitacora("Error: El cliente no está conectado.");
		            }
		            break;
		        }
		        pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	            cantidad_bytes_transferidos += numRead;

	            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
	        }
	        fclose(fileptr);
		} else {
			char peticion_realizada[80];
			memset(peticion_realizada,0,80);
			strcat(peticion_realizada,"Error de get de /videos/");
			strcat(peticion_realizada,path_video);

			escribirBitacora(peticion_realizada);

			//free(peticion_realizada);

		    fileptr = fopen("archivos_html/error_index.html", "rb");  // Open the file in binary mode
			char* fstr = "text/html";
			sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
			send(sock, salida, strlen(salida), 0);

			pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	        cantidad_bytes_transferidos += strlen(salida);

	        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

	        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
	            send(sock, salida, numRead, 0);

	            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	            cantidad_bytes_transferidos += numRead;

	            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
	        }
	        fclose(fileptr);
		}

		
	}else if ( !strncmp(salida, "GET /\0", 6) || !strncmp(salida, "GET /index.html\0", 16) ) {
		char peticion_realizada[80];
		memset(peticion_realizada,0,80);
		strcat(peticion_realizada,"Peticion get de /index.html");

		escribirBitacora(peticion_realizada);

		//free(peticion_realizada);

		fileptr = fopen("archivos_html/index.html", "rb");  
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

        cantidad_bytes_transferidos += strlen(salida);

        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0){
            send(sock, salida, numRead, 0);

            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

            cantidad_bytes_transferidos += numRead;

            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
        }
        fclose(fileptr);
	} else if ( !strncmp(salida, "GET /slideshow.html\0", 20)) {
		char peticion_realizada[80];
		memset(peticion_realizada,0,80);
		strcat(peticion_realizada,"Peticion get de /slideshow.html");

		escribirBitacora(peticion_realizada);

		fileptr = fopen("archivos_html/slideshow.html", "rb");
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

        cantidad_bytes_transferidos += strlen(salida);

        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
            send(sock, salida, numRead, 0);

            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

            cantidad_bytes_transferidos += numRead;

            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
        }
        fclose(fileptr);
	} else if ( !strncmp(salida, "GET /index_marvel.html\0", 23) ) {
		char peticion_realizada[80];
		memset(peticion_realizada,0,80);
		strcat(peticion_realizada,"Peticion get de /index_marvel.html");

		escribirBitacora(peticion_realizada);

		fileptr = fopen("archivos_html/index_marvel.html", "rb");  
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

        cantidad_bytes_transferidos += strlen(salida);

        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
            send(sock, salida, numRead, 0);

            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

            cantidad_bytes_transferidos += numRead;

            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
        }
        fclose(fileptr);
	} else if ( !strncmp(salida, "GET /slideshow_marvel.html\0", 27)) {
		char peticion_realizada[80];
		memset(peticion_realizada,0,80);
		strcat(peticion_realizada,"Peticion get de /slideshow_marvel.html");

		escribirBitacora(peticion_realizada);

		//free(peticion_realizada);
		fileptr = fopen("archivos_html/slideshow_marvel.html", "rb"); 
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

        cantidad_bytes_transferidos += strlen(salida);

        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
            send(sock, salida, numRead, 0);

            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

            cantidad_bytes_transferidos += numRead;

            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
        }
        fclose(fileptr);
	}else if (!strncmp(salida, "GET /index_dc.html\0", 19) ) {
		char peticion_realizada[80];
		memset(peticion_realizada,0,80);
		strcat(peticion_realizada,"Peticion get de /index_dc.html");

		escribirBitacora(peticion_realizada);

		fileptr = fopen("archivos_html/index_dc.html", "rb"); 
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

        cantidad_bytes_transferidos += strlen(salida);

        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

		
        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
            send(sock, salida, numRead, 0);

            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

            cantidad_bytes_transferidos += numRead;

            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
        }
        fclose(fileptr);
	} else if ( !strncmp(salida, "GET /slideshow_dc.html\0", 23)) {
		char peticion_realizada[80];
		memset(peticion_realizada,0,80);
		strcat(peticion_realizada,"Peticion get de /slideshow_dc.html");

		escribirBitacora(peticion_realizada);

		//free(peticion_realizada);

		fileptr = fopen("archivos_html/slideshow_dc.html", "rb");
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);

		pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

        cantidad_bytes_transferidos += strlen(salida);

        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
            send(sock, salida, numRead, 0);

            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

            cantidad_bytes_transferidos += numRead;

            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
        }
        fclose(fileptr);
	}else if ( !strncmp(salida, "GET /reproducir/", 16) ) {

		char video_reproducir[2000];
		char path_video[2000];
		char pagina_video[20000];

		char buffer1[20000];
		char buffer2[20000];
	
		strncpy(video_reproducir,salida+16, strlen(salida));

		char video_reproduccion[2000];

		strcat(video_reproduccion,"videos/");
		strcat(video_reproduccion,video_reproducir);

		if( access( video_reproduccion, F_OK ) != -1 ) { 
			strcat(path_video, "\"");
			strcat(path_video, "/videos/");
			strcat(path_video, video_reproducir);
			strcat(path_video, "\"");

			FILE* file_parte_alta;
			file_parte_alta = fopen(PARTEALTAVIDEO, "rb");

			while ((numRead = fread(buffer1, 1, 20000, file_parte_alta)) > 0) {
	            strcat(pagina_video, buffer1);
	        }
	        strcat(pagina_video, path_video);

	        FILE* file_parte_baja;
			file_parte_baja = fopen(PARTEBAJAVIDEO, "rb");

			while ((numRead = fread(buffer2, 1, 20000, file_parte_baja)) > 0) {
	            strcat(pagina_video, buffer2);
	        }

			char* fstr = "text/html";
			sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
			send(sock, salida, strlen(salida), 0);

			pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	        cantidad_bytes_transferidos += strlen(salida);

	        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

	        send(sock, pagina_video, strlen(pagina_video), 0);
	        pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	        cantidad_bytes_transferidos += strlen(pagina_video);

	        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

	        fclose(file_parte_alta);
	        fclose(file_parte_baja);
		} else {
			char peticion_realizada[80];
			memset(peticion_realizada,0,80);
			strcat(peticion_realizada,"Error de get de /videos/");
			strcat(peticion_realizada,path_video);

			escribirBitacora(peticion_realizada);

			fileptr = fopen("archivos_html/error_index.html", "rb");
			char* fstr = "text/html";
			sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
			send(sock, salida, strlen(salida), 0);

			pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	        cantidad_bytes_transferidos += strlen(salida);

	        pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido

	        while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
	            send(sock, salida, numRead, 0);

	            pthread_mutex_lock(&bytes_transferidos_mutex); //Bloquea el recurso compartido

	            cantidad_bytes_transferidos += numRead;

	            pthread_mutex_unlock(&bytes_transferidos_mutex); //Libera el recurso compartido
	        }
	        fclose(fileptr);
		}

		
 
	}

	free(socket_desc);
	close(sock);

	escribirBitacora("Se desconectó el cliente");

	clock_t final_consulta = clock();

    double time_spent = (double)(final_consulta - inicio_consulta) / CLOCKS_PER_SEC;

    pthread_mutex_lock(&velocidad_total_mutex); //Bloquea el recurso compartido

    velocidad_total += time_spent;

    pthread_mutex_unlock(&velocidad_total_mutex); //Libera el recurso compartido
	
	return 0;
}

void *menu_servidor(void *parametro) {
	char *mensaje_bienvenida = "Bienvenido al menu de administración del servidor";
	char *menu_encabezado = "Por favor digite el número de una de las opciones: ";
	char *parte_alta = "************************************************************";
	char *parte_menu = "****************************Menú****************************";

	char *hora_inicio = "\t1- Hora de inicio del servidor.";
	char *cantidad_bytes_transferidos_m = "\t2- Cantidad de bytes transferidos.";
	char *velocidad_promedio = "\t3- Velocidad promedio de transferencias.";
	char *cantidad_clientes = "\t4- Cantidad de clientes han consultado.";
	char *cantidad_consultas = "\t5- Cantidad de consultas realizadas.";
	char *cantidad_threads = "\t6- Cantidad de threads creados.";
	char *ver_log = "\t7- Ver log.";
	char *actualizar_archivos_index = "\t8- Actualizar archivos.";

	int opcion_menu = 0;
	printf("%s\n\n", mensaje_bienvenida);
	float velocidad_promedio_valor;
	while(1) {
		printf("%s\n", parte_alta);
		printf("%s\n", parte_menu);
		printf("%s\n\n", parte_alta);
		printf("%s\n", hora_inicio);
		printf("%s\n", cantidad_bytes_transferidos_m);
		printf("%s\n", velocidad_promedio);
		printf("%s\n", cantidad_clientes);
		printf("%s\n", cantidad_consultas);
		printf("%s\n", cantidad_threads);
		printf("%s\n", ver_log);
		printf("%s\n\n", actualizar_archivos_index);
		printf("%s", menu_encabezado);
		scanf("%d", &opcion_menu);
		printf("\n");

		switch(opcion_menu) {
	      case 1 :
	        printf("\nHora de inicio: %d-%d-%d %d:%d:%d\n\n", tiempo_inicio.tm_mday, tiempo_inicio.tm_mon + 1, tiempo_inicio.tm_year + 1900, tiempo_inicio.tm_hour, tiempo_inicio.tm_min, tiempo_inicio.tm_sec); //Corregir
	        break;
	      case 2 :
	      	printf("\nCantidad de bytes transferidos: %ld bytes\n\n", cantidad_bytes_transferidos);
	      	break;
	      case 3 :
	      	velocidad_promedio_valor = velocidad_total/cantidad_solicitudes;
	        printf("\nVelocidad promedio de transferencia: %f\n\n", velocidad_promedio_valor); //cambiar
	        break;
	      case 4 :
	        printf("\nCantidad de clientes han consultado: %d\n\n", cantidad_ip_distintas);
	        break;
	      case 5 :
	        printf("\nCantidad de consultas realizadas: %d\n\n", cantidad_solicitudes);
	        break;
	      case 6 :
	        printf("\nCantidad de threads creados: %d\n\n", cantidad_threads_creados);
	        break;
	      case 7 :
	        printf("\nLog del servidor: \n");
	        FILE* file_log;
			file_log = fopen("server.log", "rb");

			int largo_files;

			char buffer_log[1024];

			memset(buffer_log,0,1024);

			while ((largo_files = fread(buffer_log, 1, 1024, file_log)) > 0) {
		        printf("%s", buffer_log);
		        memset(buffer_log,0,1024);
		    }
		    printf("\n\n");
		    fclose(file_log);
	        break;
	      case 8 :
	      	pthread_mutex_lock(&actualizar_mutex); //Bloquea el recurso compartido

			actualizar_datos();

			pthread_mutex_unlock(&actualizar_mutex); //Libera el recurso compartido
	        printf("\nSe actualizaron los archivos...\n\n");
	        break;
	      default :
	        printf("Opción no disponible\n\n" );

	        break;
	    }
	    printf("\n");

	} 
	
}

void generar_md5sum(char* nombre_archivo) {
	char* parte1_guardar = "md5sum ";
	char* parte2_guardar = " >> ";

	directorioArchivos("videos/",&lista_videos_generar);
	directorioArchivos("images/",&lista_imagenes_generar);
	directorioArchivos("descriptions/",&lista_descripciones_generar);
	directorioArchivos("partes_html/",&lista_partes_html);

	char archivo_guardar[1000];

	for (int i = 0; i < lista_videos_generar.total; i++) {
		char* elemento_actual_video = (char *) vector_get(&lista_videos_generar, i);

		memset(archivo_guardar, 0, 1000);
		strcat(archivo_guardar, parte1_guardar);
		strcat(archivo_guardar, elemento_actual_video);
		strcat(archivo_guardar, parte2_guardar);
		strcat(archivo_guardar, nombre_archivo);

		system(archivo_guardar);

		memset(archivo_guardar, 0, 1000);

	}

	for (int i = 0; i < lista_imagenes_generar.total; i++) {
		char* elemento_actual_imagen = (char *) vector_get(&lista_imagenes_generar, i);

		strcat(archivo_guardar, parte1_guardar);
		strcat(archivo_guardar, elemento_actual_imagen);
		strcat(archivo_guardar, parte2_guardar);
		strcat(archivo_guardar, nombre_archivo);

		system(archivo_guardar);

		memset(archivo_guardar, 0, 1000);
	}

	for (int i = 0; i < lista_descripciones_generar.total; i++) {
		char* elemento_actual_descripcion = (char *) vector_get(&lista_descripciones_generar, i);

		strcat(archivo_guardar, parte1_guardar);
		strcat(archivo_guardar, elemento_actual_descripcion);
		strcat(archivo_guardar, parte2_guardar);
		strcat(archivo_guardar, nombre_archivo);

		system(archivo_guardar);

		memset(archivo_guardar, 0, 1000);
	}

	for (int i = 0; i < lista_partes_html.total; i++) { 

		char* elemento_actual_partes_html = (char *) vector_get(&lista_partes_html, i);

		strcat(archivo_guardar, parte1_guardar);
		strcat(archivo_guardar, elemento_actual_partes_html);
		strcat(archivo_guardar, parte2_guardar);
		strcat(archivo_guardar, nombre_archivo);

		system(archivo_guardar);

		memset(archivo_guardar, 0, 1000);
	}

	memset(archivo_guardar, 0, 1000);

	strcat(archivo_guardar, parte1_guardar);
	strcat(archivo_guardar, "datos_paginas/archivos_datos");
	strcat(archivo_guardar, parte2_guardar);
	strcat(archivo_guardar, nombre_archivo);
	
	system(archivo_guardar);

	memset(archivo_guardar, 0, 1000);

	strcat(archivo_guardar, parte1_guardar);
	strcat(archivo_guardar, "datos_paginas/archivos_datos_marvel");
	strcat(archivo_guardar, parte2_guardar);
	strcat(archivo_guardar, nombre_archivo);
	
	system(archivo_guardar);

	memset(archivo_guardar, 0, 1000);

	strcat(archivo_guardar, parte1_guardar);
	strcat(archivo_guardar, "datos_paginas/archivos_datos_dc");
	strcat(archivo_guardar, parte2_guardar);
	strcat(archivo_guardar, nombre_archivo);
	
	system(archivo_guardar);
}

void generar_archivos_cantidad() {
	int cantidad_video = directorio_archivos_cantidad("videos/");
	char tamanho_video[10];
	memset(tamanho_video,0,10);
	sprintf(tamanho_video,"%d",cantidad_video);

	FILE *file_video = fopen("cantidad_video", "w+");
    if (file_video != NULL) {
        fputs(tamanho_video, file_video);
        fclose(file_video);
    } else {
    	printf("Error\n");
    }

	int cantidad_imagen = directorio_archivos_cantidad("images/");
	char tamanho_imagen[10];
	memset(tamanho_imagen,0,10);
	sprintf(tamanho_imagen,"%d",cantidad_imagen);

	FILE *file_imagen = fopen("cantidad_imagen", "w+");
    if (file_imagen != NULL) {
        fputs(tamanho_imagen, file_imagen);
        fclose(file_imagen);
    } else {
    	printf("Error\n");
    }

	int cantidad_descripcion = directorio_archivos_cantidad("descriptions/");
	char tamanho_descripcion[10];
	memset(tamanho_descripcion,0,10);
	sprintf(tamanho_descripcion,"%d",cantidad_descripcion);

	FILE *file_descripcion = fopen("cantidad_descripcion", "w+");
    if (file_descripcion != NULL) {
        fputs(tamanho_descripcion, file_descripcion);
        fclose(file_descripcion);
    } else {
    	printf("Error\n");
    }

	int cantidad_datos = directorio_archivos_cantidad("datos_paginas/");
	char tamanho_datos[10];
	memset(tamanho_datos,0,10);
	sprintf(tamanho_datos,"%d",cantidad_datos);

	FILE *file_datos = fopen("cantidad_datos", "w+");
    if (file_datos != NULL) {
        fputs(tamanho_datos, file_datos);
        fclose(file_datos);
    } else {
    	printf("Error\n");
    }
}

int revisar_cantidad_archivos() {
	
	int cantidad_video = directorio_archivos_cantidad("videos/");
	char tamanho_video[10];
	memset(tamanho_video,0,10);
	sprintf(tamanho_video,"%d",cantidad_video);

	FILE *archivo_tamanho_video = fopen("cantidad_video","r");
	char tamanho_video_comparar[10];
	memset(tamanho_video_comparar,0,10);
	fread(tamanho_video_comparar, 1, 10, archivo_tamanho_video);

	int cantidad_imagen = directorio_archivos_cantidad("images/");
	char tamanho_imagen[10];
	memset(tamanho_imagen,0,10);
	sprintf(tamanho_imagen,"%d",cantidad_imagen);

	FILE *archivo_tamanho_imagen = fopen("cantidad_imagen","r");
	char tamanho_imagen_comparar[10];
	memset(tamanho_imagen_comparar,0,10);
	fread(tamanho_imagen_comparar, 1, 10, archivo_tamanho_imagen);

	int cantidad_descripcion = directorio_archivos_cantidad("descriptions/");
	char tamanho_descripcion[10];
	memset(tamanho_descripcion,0,10);
	sprintf(tamanho_descripcion,"%d",cantidad_descripcion);

	FILE *archivo_tamanho_descripcion = fopen("cantidad_descripcion","r");
	char tamanho_descripcion_comparar[10];
	memset(tamanho_descripcion_comparar,0,10);
	fread(tamanho_descripcion_comparar, 1, 10, archivo_tamanho_descripcion);

	int cantidad_datos = directorio_archivos_cantidad("datos_paginas/");
	char tamanho_datos[10];
	memset(tamanho_datos,0,10);
	sprintf(tamanho_datos,"%d",cantidad_datos);

	FILE *archivo_tamanho_datos = fopen("cantidad_datos","r");
	char tamanho_datos_comparar[10];
	memset(tamanho_datos_comparar,0,10);
	fread(tamanho_datos_comparar, 1, 10, archivo_tamanho_datos);

	fclose(archivo_tamanho_video);
	fclose(archivo_tamanho_descripcion);
	fclose(archivo_tamanho_imagen);
	fclose(archivo_tamanho_datos);

	if (strlen(tamanho_video_comparar) != strlen(tamanho_video) || strncmp(tamanho_video_comparar, tamanho_video, strlen(tamanho_video)) ) {
		return 0;
	} else if (strlen(tamanho_descripcion_comparar) != strlen(tamanho_descripcion) || strncmp(tamanho_descripcion_comparar, tamanho_descripcion, strlen(tamanho_descripcion)) ) {
		return 0;
	} else if (strlen(tamanho_imagen_comparar) != strlen(tamanho_imagen) || strncmp(tamanho_imagen_comparar, tamanho_imagen, strlen(tamanho_imagen)) ) {
		return 0;
	} else if (strlen(tamanho_datos_comparar) != strlen(tamanho_datos) || strncmp(tamanho_datos_comparar, tamanho_datos, strlen(tamanho_datos)) ) {
		return 0;
	} else {
		return 1;
	}


}

void revisar_generacion_index(char* nombre_archivo) {
	
	char* parte1_comando = "md5sum -c --status ";
	char* parte2_comando = ">>/dev/null 2>>/dev/null";
	char archivo_consultar[100];

	memset(archivo_consultar, 0, 100);

	strcat(archivo_consultar, parte1_comando);
	strcat(archivo_consultar, nombre_archivo);
	strcat(archivo_consultar,parte2_comando);

	if (access( nombre_archivo, F_OK ) != -1) {
		if ( !revisar_cantidad_archivos() || (system(archivo_consultar)) != 0) {
			
			char borrar_archivo[20];
			memset(borrar_archivo,0,20);
			strcat(borrar_archivo,"rm ");
			strcat(borrar_archivo,nombre_archivo);
			system(borrar_archivo);
			
			generar_index(pagina_index,&lista_videos,&lista_imagenes,&lista_descripciones,"archivos_html/index.html");
			generar_index(pagina_index_marvel,&lista_marvel_videos,&lista_marvel_imagenes,&lista_marvel_descripciones,"archivos_html/index_marvel.html");
			generar_index(pagina_index_dc,&lista_dc_videos,&lista_dc_imagenes,&lista_dc_descripciones,"archivos_html/index_dc.html");		
			generar_index_slideshow(pagina_index_slice,&lista_videos,&lista_imagenes,&lista_descripciones,"archivos_html/slideshow.html");
			generar_index_slideshow(pagina_index_slice_marvel,&lista_marvel_videos,&lista_marvel_imagenes,&lista_marvel_descripciones,"archivos_html/slideshow_marvel.html");
			generar_index_slideshow(pagina_index_slice_dc,&lista_dc_videos,&lista_dc_imagenes,&lista_dc_descripciones,"archivos_html/slideshow_dc.html");
			
			generar_md5sum(nombre_archivo);
		
			generar_archivos_cantidad();

		} else if (access( "archivos_html/index.html", F_OK ) == -1 || access( "archivos_html/slideshow.html", F_OK ) == -1 || access( "archivos_html/index_marvel.html", F_OK ) == -1 || access( "archivos_html/slideshow_marvel.html", F_OK ) == -1 || access( "archivos_html/index_dc.html", F_OK ) == -1 || access( "archivos_html/slideshow_dc.html", F_OK ) == -1) {
			
			char borrar_archivo[20];
			strcat(borrar_archivo,"rm ");
			strcat(borrar_archivo,nombre_archivo);
			system(borrar_archivo);

			generar_index(pagina_index,&lista_videos,&lista_imagenes,&lista_descripciones,"archivos_html/index.html");
			generar_index(pagina_index_marvel,&lista_marvel_videos,&lista_marvel_imagenes,&lista_marvel_descripciones,"archivos_html/index_marvel.html");
			generar_index(pagina_index_dc,&lista_dc_videos,&lista_dc_imagenes,&lista_dc_descripciones,"archivos_html/index_dc.html");		

			generar_index_slideshow(pagina_index_slice,&lista_videos,&lista_imagenes,&lista_descripciones,"archivos_html/slideshow.html");
			generar_index_slideshow(pagina_index_slice_marvel,&lista_marvel_videos,&lista_marvel_imagenes,&lista_marvel_descripciones,"archivos_html/slideshow_marvel.html");
			generar_index_slideshow(pagina_index_slice_dc,&lista_dc_videos,&lista_dc_imagenes,&lista_dc_descripciones,"archivos_html/slideshow_dc.html");

			generar_md5sum(nombre_archivo);
			generar_archivos_cantidad();
		}
	} else {
		
		generar_index(pagina_index,&lista_videos,&lista_imagenes,&lista_descripciones,"archivos_html/index.html");
		generar_index(pagina_index_marvel,&lista_marvel_videos,&lista_marvel_imagenes,&lista_marvel_descripciones,"archivos_html/index_marvel.html");
		generar_index(pagina_index_dc,&lista_dc_videos,&lista_dc_imagenes,&lista_dc_descripciones,"archivos_html/index_dc.html");		
		generar_index_slideshow(pagina_index_slice,&lista_videos,&lista_imagenes,&lista_descripciones,"archivos_html/slideshow.html");
		generar_index_slideshow(pagina_index_slice_marvel,&lista_marvel_videos,&lista_marvel_imagenes,&lista_marvel_descripciones,"archivos_html/slideshow_marvel.html");
		generar_index_slideshow(pagina_index_slice_dc,&lista_dc_videos,&lista_dc_imagenes,&lista_dc_descripciones,"archivos_html/slideshow_dc.html");

		generar_md5sum(nombre_archivo);
		generar_archivos_cantidad();
	
	}

}

void directorioArchivos(char* direccion,vector *lista_caracteres) {
    struct dirent *de;  
  
    DIR *dr = opendir(direccion);

    if (dr == NULL) { 
        printf("Could not open current directory" ); 
    } 
    
    while ((de = readdir(dr)) != NULL) {
        if (strncmp(de->d_name, ".\0", 2) && strncmp(de->d_name, "..\0", 3)) {
            char* buffer = malloc(100);
           	memset(buffer,0,100);
            strcat(buffer, direccion);
            strcat(buffer, de->d_name);
          
            vector_add(lista_caracteres, buffer);
        }
    }     
  
    closedir(dr);
}

int directorio_archivos_cantidad(char* direccion) {
    struct dirent *de;  
  	int cantidad_final = 0;
    DIR *dr = opendir(direccion);

    if (dr == NULL) { 
        printf("Could not open current directory" ); 
    } 
    
    while ((de = readdir(dr)) != NULL) {
        if (strncmp(de->d_name, ".\0", 2) && strncmp(de->d_name, "..\0", 3)) {
            cantidad_final += 1;
        }
    }     
  
    closedir(dr);

    return cantidad_final;
}

void escribirBitacora(char* dato) {

	pthread_mutex_lock(&log_mutex); //Bloquea el recurso compartido
	FILE *file_bitacora = fopen("server.log", "a");
	
	fputs(dato,file_bitacora);
	fputs("\n",file_bitacora);

	fclose(file_bitacora);

	pthread_mutex_unlock(&log_mutex); //Libera el recurso compartido

}

void leer_archivo_datos(char* archivo,vector* lista_archivos_description,vector* lista_archivos_video,vector* lista_archivos_image) {
	FILE *fileptr = NULL;
	char * line = NULL;
    size_t len = 0;
    ssize_t read;

	fileptr = fopen(archivo, "r");

	while ((read = getline(&line, &len, fileptr)) != -1) {

		char palabra[100];
		memset(palabra, 0, 100);
		strncpy(palabra,line, strlen(line)-1);

        char* archivo_seleccionado_description = malloc(100);
        memset(archivo_seleccionado_description, 0, 100);
        strcat(archivo_seleccionado_description, "descriptions/");
        strcat(archivo_seleccionado_description,palabra);
        strcat(archivo_seleccionado_description,"");

        char* archivo_seleccionado_images = malloc(100);
        memset(archivo_seleccionado_images, 0, 100);
        strcat(archivo_seleccionado_images, "images/");
        strcat(archivo_seleccionado_images,palabra);
        strcat(archivo_seleccionado_images,".png");

        char archivo_seleccionado_videos[100];
        memset(archivo_seleccionado_videos, 0, 100);
        strcat(archivo_seleccionado_videos, "videos/");
        strcat(archivo_seleccionado_videos,palabra);
        strcat(archivo_seleccionado_videos,".mp4");

        if (access( archivo_seleccionado_description, F_OK ) != -1 && access( archivo_seleccionado_videos, F_OK ) != -1 && access( archivo_seleccionado_images, F_OK ) != -1) {
        	char* archivo_seleccionado_reproducir = malloc(100);
	        memset(archivo_seleccionado_reproducir, 0, 100);
	        strcat(archivo_seleccionado_reproducir, "reproducir/");
	        strcat(archivo_seleccionado_reproducir,palabra);
	        strcat(archivo_seleccionado_reproducir,".mp4");

        	vector_add(lista_archivos_description, archivo_seleccionado_description);
        	vector_add(lista_archivos_video, archivo_seleccionado_reproducir);
        	vector_add(lista_archivos_image, archivo_seleccionado_images);

        }
  
    }

    fclose(fileptr);
}

void *actualizar_index(void *parametro) {
	while(1) {

 		sleep(15);

 		pthread_mutex_lock(&actualizar_mutex); //Bloquea el recurso compartido

		actualizar_datos();

		pthread_mutex_unlock(&actualizar_mutex); //Libera el recurso compartido
		

	}
	
}

void actualizar_datos() {
	memset(pagina_index, 0, BUFFER_SIZE_INDEX);
    memset(pagina_index_slice, 0, BUFFER_SIZE_INDEX);
       
    memset(pagina_index_marvel, 0, BUFFER_SIZE_INDEX);
    memset(pagina_index_slice_marvel, 0, BUFFER_SIZE_INDEX); 
  
    memset(pagina_index_dc, 0, BUFFER_SIZE_INDEX);
    memset(pagina_index_slice_dc, 0, BUFFER_SIZE_INDEX);

	vector_free(&lista_videos);
    vector_free(&lista_imagenes);
    vector_free(&lista_descripciones);

    vector_free(&lista_marvel_videos);
    vector_free(&lista_marvel_imagenes);
    vector_free(&lista_marvel_descripciones);

    vector_free(&lista_dc_videos);
    vector_free(&lista_dc_imagenes);
    vector_free(&lista_dc_descripciones);

    cargar_archivos_vectores();

    vector_free(&lista_videos_generar);
	vector_free(&lista_imagenes_generar);
	vector_free(&lista_descripciones_generar);
	vector_free(&lista_partes_html);

	vector_init(&lista_videos_generar);
	vector_init(&lista_imagenes_generar);
	vector_init(&lista_descripciones_generar);
	vector_init(&lista_partes_html);

    revisar_generacion_index("datos_revision.md5");
}

void cargar_archivos_vectores() {

	vector_init(&lista_videos);
    vector_init(&lista_imagenes);
    vector_init(&lista_descripciones);

    vector_init(&lista_marvel_videos);
    vector_init(&lista_marvel_imagenes);
    vector_init(&lista_marvel_descripciones);

    vector_init(&lista_dc_videos);
    vector_init(&lista_dc_imagenes);
    vector_init(&lista_dc_descripciones);

	leer_archivo_datos("datos_paginas/archivos_datos",&lista_descripciones,&lista_videos,&lista_imagenes);

	leer_archivo_datos("datos_paginas/archivos_datos_marvel",&lista_marvel_descripciones,&lista_marvel_videos,&lista_marvel_imagenes);

	leer_archivo_datos("datos_paginas/archivos_datos_dc",&lista_dc_descripciones,&lista_dc_videos,&lista_dc_imagenes);

}

long int findSize(char* file_name) { 
   
    FILE* fp = fopen(file_name, "r"); 
  
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    } 
  
    fseek(fp, 0L, SEEK_END); 
  
    long int res = ftell(fp); 
  
    fclose(fp); 
  
    return res; 
} 

void generar_index_slideshow(char* pagina_generar,vector* lista_videos_generar,vector* lista_imagenes_generar,vector* lista_descripciones_generar,char* nombre_archivo) {
	char buffer1[6000];

	memset(buffer1, 0, 6000);

	FILE* file_parte_alta_slide;
	file_parte_alta_slide = fopen(PARTEALTASLIDESHOW, "rb");

	int largo_files;

	while ((largo_files = fread(buffer1, 1, 6000, file_parte_alta_slide)) > 0) {
        strcat(pagina_generar, buffer1);
    }

    char * parteASlideshow = "<ol class=\"carousel-indicators\">";
    char * parteBSlideshow = "</ol>\n<div class=\"carousel-inner\">\n";

    char *parteATarjets = "<li data-target=\"#myCarousel\" data-slide-to=";
    char *parteBTarjets = "></li>";

    char *parteActive = " class=\"active\"";

    strcat(pagina_generar, parteASlideshow);

    for(int i = 0; i < lista_videos_generar->total; i++) {
    	char numero[4];
    	memset(numero, 0, 4);

    	sprintf(numero, "\"%d\"",i);  	
    	if(!i) {
    		strcat(pagina_generar, parteATarjets);
    		strcat(pagina_generar,numero);
    		strcat(pagina_generar, parteActive); 	
    		strcat(pagina_generar, parteBTarjets); 		
    	} else {
    		strcat(pagina_generar, parteATarjets);
    		strcat(pagina_generar,numero); 	
    		strcat(pagina_generar, parteBTarjets);
    	}
    }

    strcat(pagina_generar, parteBSlideshow);

    char* parteAEncabezado = "<div class=\"item\">\n";
    char* parteAEncabezadoActivo = "<div class=\"item active\"\n>";

    char* parteAVideo = "<a href=";
    char* parteBVideo = ">\n";

    char* parteAImagen = "<img src=";
    char* parteBImagen = " alt=\"Los Angeles\"  width=\"1000\" height=\"400\">";

    char* parteADescripcion = "<div class=\"carousel-caption\">\n<p>";
    char* parteBDescripcion = "</p style=\"background-color: coral;\">\n</div>\n</a>\n</div>\n";

    for (int i = 0; i <  lista_videos_generar->total; i++) {
        char* video_path = (char *) vector_get(lista_videos_generar, i);
        char* imagen_path = (char *) vector_get(lista_imagenes_generar, i);
        char* descripcion_path = (char *) vector_get(lista_descripciones_generar, i);

        if (!i) {
        	strcat(pagina_generar, parteAEncabezadoActivo);
        } else {
        	strcat(pagina_generar, parteAEncabezado);
        }

  
        strcat(pagina_generar, parteAVideo);
        strcat(pagina_generar, video_path);
        strcat(pagina_generar, parteBVideo);

        strcat(pagina_generar, parteAImagen);
        strcat(pagina_generar, imagen_path);
        strcat(pagina_generar, parteBImagen);

        strcat(pagina_generar, parteADescripcion);

        if( access( descripcion_path, F_OK ) != -1 ) { 
        	FILE* file_descripcion;
	        file_descripcion = fopen(descripcion_path, "rb");

	        char buffer3[1024];
	        memset(buffer3, 0, 1024);

	        while ((largo_files = fread(buffer3, 1, 1024, file_descripcion)) > 0) {
	            strcat(pagina_generar, buffer3);
	        }

	        char tamanho_archivo[80];

	        char archivo_descripcion[30];

	        char video_path_descripcion[50];

	        memset(archivo_descripcion,0,30);

	        strncpy(archivo_descripcion,descripcion_path+13, strlen(descripcion_path));

	        memset(&file_datos,0,sizeof(stat));

	        memset(video_path_descripcion,0,sizeof(video_path_descripcion));

	        strcat(video_path_descripcion,"videos/");
	        strcat(video_path_descripcion,archivo_descripcion);
	        strcat(video_path_descripcion,".mp4");

	        stat(video_path_descripcion,&file_datos);

	        long int largo = findSize(video_path_descripcion);

	        memset(tamanho_archivo,0,80);

	        sprintf(tamanho_archivo,"Tamaño del archivo: %ld bytes - Fecha de modificación: %s",largo,ctime(&file_datos.st_mtime)); 

	        strcat(pagina_generar,tamanho_archivo);

        } else {
        	char *no_descripcion = "No tiene una descripcion";
        	strcat(pagina_generar, no_descripcion);
        }

        strcat(pagina_generar, parteBDescripcion);

    }

    FILE* file_parte_baja_slide;
	file_parte_baja_slide = fopen(PARTEBAJASLIDESHOW, "rb");

	memset(buffer1, 0, 6000);

	while ((largo_files = fread(buffer1, 1, 6000, file_parte_baja_slide)) > 0) {
        strcat(pagina_generar, buffer1);
    }

    FILE *file_index = fopen(nombre_archivo, "w+");
    if (file_index != NULL) {
        fputs(pagina_generar, file_index);
        fclose(file_index);
    } else {
    	printf("Error2\n");
    }

}

void generar_index(char* pagina_generar,vector* lista_videos_generar,vector* lista_imagenes_generar,vector* lista_descripciones_generar,char* nombre_archivo) {
	char buffer1_index[6000];

	memset(buffer1_index, 0, 6000);

	FILE* file_parte_alta;
	file_parte_alta = fopen(PARTEALTAINDEX, "rb");

	int largo_files;

	while ((largo_files = fread(buffer1_index, 1, 6000, file_parte_alta)) > 0) {
        strcat(pagina_generar, buffer1_index);
    }

    char* parteAEncabezado = "<div class=\"col-md-4\">\n\t<div class=\"thumbnail\">";
    char* parteAVideo = "<a href=";
    char* parteBVideo = ">\n";

    char* parteAImagen = "<img src=";
    char* parteBImagen = " alt=\"Lights\" style=\"width:100%\">";

    char* parteADescripcion = "<div class=\"caption\">\n\t<p>";
    char* parteBDescripcion = "</p>\n</div>\n</a>\n</div>\n</div>";

    for (int i = 0; i <  lista_videos_generar->total; i++) {
        char* video_path = (char *) vector_get(lista_videos_generar, i);
        char* imagen_path = (char *) vector_get(lista_imagenes_generar, i);
        char* descripcion_path = (char *) vector_get(lista_descripciones_generar, i);

        strcat(pagina_generar, parteAEncabezado);

        strcat(pagina_generar, parteAVideo);
        strcat(pagina_generar, video_path);
        strcat(pagina_generar, parteBVideo);

        strcat(pagina_generar, parteAImagen);
        strcat(pagina_generar, imagen_path);
        strcat(pagina_generar, parteBImagen);

        strcat(pagina_generar, parteADescripcion);

        if( access( descripcion_path, F_OK ) != -1 ) {
        	FILE* file_descripcion;
	        file_descripcion = fopen(descripcion_path, "rb");

	        char buffer3_index[1024];
	        memset(buffer3_index, 0, 1024);

	        while ((largo_files = fread(buffer3_index, 1, 1024, file_descripcion)) > 0) {
	            strcat(pagina_generar, buffer3_index);
	        }

	        char tamanho_archivo[80];

	        char archivo_descripcion[30];

	        char video_path_descripcion[50];

	        memset(archivo_descripcion,0,30);

	        strncpy(archivo_descripcion,descripcion_path+13, strlen(descripcion_path));

	        memset(&file_datos,0,sizeof(stat));

	        memset(video_path_descripcion,0,50);

	        strcat(video_path_descripcion,"videos/");
	        strcat(video_path_descripcion,archivo_descripcion);
	        strcat(video_path_descripcion,".mp4");

	        stat(video_path_descripcion,&file_datos);

	        long int largo = findSize(video_path_descripcion);

	        memset(tamanho_archivo,0,80);

	        sprintf(tamanho_archivo,"Tamaño del archivo: %ld bytes - Fecha de modificación: %s",largo,ctime(&file_datos.st_mtime)); 

	        strcat(pagina_generar,tamanho_archivo);


        } else {
        	char *no_descripcion = "No tiene una descripcion";
        	strcat(pagina_generar, no_descripcion);
        }

        strcat(pagina_generar, parteBDescripcion);

    }

    FILE* file_parte_baja;
	file_parte_baja = fopen(PARTEBAJAINDEX, "rb");

	memset(buffer1_index, 0, 6000);

	while ((largo_files = fread(buffer1_index, 1, 6000, file_parte_baja)) > 0) {
        strcat(pagina_generar, buffer1_index);
    }

    FILE *fp = fopen(nombre_archivo, "w+");
    if (fp != NULL) {
        fputs(pagina_generar, fp);
        fclose(fp);
    } else {
    	printf("Error\n");
    }

}

int contains(vector* lista, char* elemento) {

	for(int i = 0; i < lista->total; i++) {
		char* elemento_actual = (char *) vector_get(lista, i);
		if( !strncmp(elemento_actual, elemento, strlen(elemento)) ) {
			return 1;
		} 
	}

	return 0;
}

int end_with_mp4( char *string ){
  string = strrchr(string, '.');

  if( string != NULL )
    return ( strcmp(string, "mp4") );

  return ( -1 );
}
