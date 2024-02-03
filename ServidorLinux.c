// Server

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/time.h> 

#define PORT 			4432
#define MAX			10
#define BUFFER_SIZE 		1024

#define USUARI 			"jordint"
#define CLAU			"passw0rd"


typedef enum {
    lliure = 0,
    ocupat = 1
} e_estat;

typedef struct {
    int sock;
    e_estat estat;
    int a, b, result;
    int fase;
} t_client;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void *run(void *d) {
    t_client *client = (t_client *)d;
    int operation, result = 0;
    int m, err;
    fd_set rfds, wfds;
    struct timeval tv;
    int cli;
    int retval;

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    for (;;) {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        for (int i = 0; i < MAX; i++) {
            if (client[i].estat == ocupat) {
                cli = client[i].sock;
                switch (client[i].fase) {
                    case 0:
                    case 1:
                        FD_SET(cli, &rfds);
                        break;
                    case 2:
                        FD_SET(cli, &wfds);
                        break;
                }
            }
        }

        retval = select(MAX + 3, &rfds, &wfds, NULL, &tv);

        for (int i = 0; i < MAX; i++) {
            if (client[i].estat == ocupat) {
                cli = client[i].sock;
                switch (client[i].fase) {
                    case 0:
                        if (FD_ISSET(cli, &rfds)) {
                            m = read(cli, &(client[i].a), sizeof(int));
                            client[i].fase = 1;
                            break;
                        }
                    case 1:
                        if (FD_ISSET(cli, &rfds)) {
                            m = read(cli, &(client[i].b), sizeof(int));
                            client[i].fase = 2;
                            break;
                        }

                    case 2:
                        if (FD_ISSET(cli, &wfds)) {
                            client[i].result = client[i].a + client[i].b;
                            m = write(cli, &(client[i].result), sizeof(int));

                            // Tancar i alliberar
                            close(cli);
                            pthread_mutex_lock(&mut);
                            client[i].estat = lliure;
                            pthread_mutex_unlock(&mut);
                            break;
                        }
                }
            }
        }
    }

    pthread_exit(NULL);
}

void init_clients(t_client *cli) {
    int i;

    for (i = 0; i < MAX; i++) {
        cli[i].estat = lliure;
    }
}

int get_lliure(t_client *cli) {
    int i;

    for (i = 0; i < MAX; i++) {
        if (cli[i].estat == lliure)
            return i;
    }
    return -1;
}

int main(int argc, char **argv) {
    int sockfd, cli, len, pid, result, pos;
    struct sockaddr_in server, client;
    pthread_t server_thread;
    char buffer[BUFFER_SIZE];

    t_client clients[MAX];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(sockfd, (struct sockaddr *)&server, sizeof(server))) < 0) {
        perror("bind");
        return 1;
    }

    if ((listen(sockfd, 10)) < 0) {
        perror("listen");
        return 1;
    }

    len = sizeof(client);

    init_clients(clients);

    if (pthread_create(&server_thread, NULL, run, (void *)clients) != 0) {
        perror("pthread_create");
        return 1;
    }

    for (;;) {
        cli = accept(sockfd, (struct sockaddr *)&client, &len);

        if (cli < 0) {
            perror("accept");
            return 1;
        }

        /*
            Gestionar la entrada de usuari i clau de pas per a permetre 
            o no l'entrada al servidor.
        */

        printf("Connexió establerta...\n");
        ssize_t bytes_received_usuari = recv(cli, buffer, sizeof(buffer), 0);
        if(bytes_received_usuari == -1) {
            perror("Error al rebre dades");
            return 1;
        }

	const char* usuari_entrat = strdup(buffer);

        printf("Dades rebudes: %s\n", usuari_entrat);
        memset(buffer, '\0', sizeof(buffer));
        
        
        ssize_t bytes_received_clau = recv(cli, buffer, sizeof(buffer), 0);
        if(bytes_received_clau == -1) {
            perror("Error al rebre dades");
            return 1;
        }
        
        const char *clau_entrada = strdup(buffer);
        printf("Dades rebudes: %s\n", clau_entrada);
        
        memset(buffer, '\0', sizeof(buffer));

	// Validació del usuari i la clau de pas:
	       
       	if(strcmp(usuari_entrat, USUARI) == 0) {
       		if(strcmp(clau_entrada, CLAU) == 0) {
       			const char* msg = "0";
       			ssize_t bytes_send_auth = send(cli, msg, strlen(msg), 0);
       			if(bytes_send_auth == -1) {
       				perror("Error al enviar el missatge de verificació del login.");
       				return 1;
       			}
       		}
       	}



	////////////////////////////////////////////////////////////////////////////////////////
	

        pthread_mutex_lock(&mut);
        pos = get_lliure(clients);
        if (pos > -1) {
            clients[pos].estat = ocupat;
            clients[pos].sock = cli;
            clients[pos].fase = 0;
        }
        pthread_mutex_unlock(&mut);

        if (pos == -1) {
            close(cli);
        }
    }

    close(sockfd);

    return 0;
}
