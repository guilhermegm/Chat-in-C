#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold

#define NUM_THREADS 50
#define MAXBUFSIZE 500

typedef struct _tArgs {
  int tid;
  int sid;
}TArgs;

typedef struct _conexao {
  int tid;
  int sid;
  struct _conexao *prox;
}Conexao;

Conexao *cn;

void *trataConexao(void *threadid);
void *msgRecebida(void *tid);
Conexao *insereConexaoInicio(int tid, int sid);
Conexao *novaConexao(int tid, int sid);

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(void) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
  
    pthread_t threads[NUM_THREADS];

    int rc;
    long t;
    
    t = 0;
    cn = NULL;
    
    printf("server: waiting for connections...\n");
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
	
	TArgs args;
	args.tid = t;
	args.sid = new_fd;
	rc = pthread_create(&threads[t], NULL, trataConexao, (void *)&args);
	if(cn == NULL)
	  cn = insereConexaoInicio(args.tid, args.sid);
	else
	  insereConexaoFinal(cn, args.tid, args.sid);

	t++;
	/*Conexao *cnAux = cn;
	while(cnAux != NULL) {
	  printf("{tid: %d, sid: %d}\n", cnAux->tid, cnAux->sid);
	  cnAux = cnAux->prox;
	}
	
	int i;
	for(i=0; i<5; i++) {
	  if(&threads[i] == NULL)
	    printf("NULL: [%d]\n", i);
	}*/
	
	/*if(rc)
	  printf("ERROR; thread");
	else
	  t++;*/
	
        /*if (!fork()) { // this is the child process
             // child doesn't need the listener
            printf("server: lost connection from %s\n", s);
            close(new_fd);
            exit(0);
        }*/
    }
    
    pthread_exit(NULL);
    close(sockfd);
    return 0;
}

void *trataConexao(void *targs) {
  TArgs *args = (TArgs *)targs;
  char buf[MAXBUFSIZE];
  int rc, t;
  pthread_t tMsgRecebida;
  
  t = args->sid;
  rc = pthread_create(&tMsgRecebida, NULL, msgRecebida, (void *)t);

  //sprintf(buf, "%ld\0", tid); 
  //send(new_fd, buf, strlen(buf)+1, 0);
  pthread_exit(NULL);
  close(args->sid);
}

Conexao *insereConexaoInicio(int tid, int sid) {
  Conexao *cn = novaConexao(tid, sid);
  
  return cn;
}

void *msgRecebida(void *sid) {
  int sockid = (int)sid;
  int numbytes;
  char buf[MAXBUFSIZE];
  char buf2[MAXBUFSIZE + 20];
  
  //Quando eu desconecto do servidor pelo cliente o servidor termina a minha threads e socket, corretamente?
  while(1) {
    if((numbytes = recv(sockid, buf, MAXBUFSIZE, 0)) != -1) {
      sprintf(buf2, "%d, diz: %s\n", sockid, buf);
      printf("%s", buf2);
      Conexao *cnAux = cn;
      while(cnAux != NULL) {
	send(cnAux->sid, buf2, strlen(buf2)+1, 0);
	
	cnAux = cnAux->prox;
      }
    }
  }
  pthread_exit(NULL);
}

int insereConexaoFinal(Conexao *cn, int tid, int sid) {
  Conexao *cnAux = cn;
  while(cnAux->prox != NULL)
    cnAux = cnAux->prox;
  
  Conexao *novo = novaConexao(tid, sid);
  cnAux->prox = novo;
  
  return 1;
}

Conexao *novaConexao(int tid, int sid) {
  Conexao *novo = (Conexao *)malloc(sizeof(Conexao));
  novo->tid = tid;
  novo->sid = sid;
  novo->prox = NULL;
  
  return novo;
}