
// Client

#define PORT 4432

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <curl/curl.h>

#define MAXSTRING 100

int run(int sockfd) {
    int result = 0;
    char buffer[MAXSTRING];
    
    // Sol·licito l'usuari i l'envio cap el servidor:
    printf("Introdueix l'usuari:\n");
    scanf("%s", buffer);
    const char* usuario = strdup(buffer);
    
    ssize_t bytes_send_usuario = send(sockfd, usuario, strlen(usuario), 0);
    if (bytes_send_usuario == -1) {
        perror("Error al enviar l'usuari al servidor");
        result = 1;
    } 
    
    memset(buffer, '\0', sizeof(buffer));
    // Sol·licito la clau i l'envio cap el servidor:
    
    printf("Introdueix la clau d'access:\n");
    scanf("%s", buffer);
    const char* clave = strdup(buffer); 
    
    ssize_t bytes_send_clave = send(sockfd, clave, strlen(clave), 0);
    if(bytes_send_clave == -1) {
    	perror("Error al enviar la clau al servidor");
    	return 1;
    } 
    memset(buffer, '\0', sizeof(buffer));
    
    printf("Autenticació enviada al servidor...\n\n");
    
    
    // Esperar la resposta del servidor:
    
    ssize_t bytes_rcv_auth = recv(sockfd, buffer, sizeof(buffer), 0);
    if(bytes_rcv_auth == -1) {
    	perror("Error en rebre la resposta del login.");
    	return 1;
    }
    
    buffer[bytes_rcv_auth] = '\0';
    
    if(strcmp(buffer, "0") != 0) {
        // Si l'autenticació és incorrecte, l'aplicació s'acaba.
    	printf("Autenticació incorrecte.\n");
    	return 0;
    } else {
    	printf("Auntenticació correcte.\n");
    }

    printf("Introdueix la ruta del arxiu que vols enviar: ");
    scanf("%s", buffer);
    const char *file_path = strdup(buffer);    

    FILE *file = fopen(file_path, "rb");
    if(file == NULL) {
        perror("Error al obrir l'arxiu a enviar");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    ssize_t bytes_sent_size = send(sockfd, &file_size, sizeof(file_size), 0);
    if(bytes_sent_size == -1) {
        perror("Error al enviar les dimensions del arxiu al servidor...");
        fclose(file);
        free((void *)file_path);
        return 1;
    }

    ssize_t bytes_sent_file;
    while((bytes_sent_file = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if(send(sockfd, buffer, bytes_sent_file, 0) == -1) {
            perror("Error al enviar l'arxiu al servidor");
            fclose(file);
            free((void *)file_path);
            return 1;
        }
    }

    fclose(file);
    printf("Arxiu enviat correctament.\n");
    
    free((void *)file_path);
    free((void*)usuario);
    free((void*)clave);

    return result;
}

int main(int argc, char** argv) {
    int sockfd, m;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        return 1;
    }

    run(sockfd);

    close(sockfd);

    return 0;
}
