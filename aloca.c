#include <stdio.h>

#define ALLOCSIZE 101

typedef unsigned char byte;

byte buf[ALLOCSIZE];

int verificaBufEspaco(int i, int n);
void *aloca(int n);
void desaloca(void *p);

int main(void) {
  int *a = (int *)aloca(sizeof(int));
  *a = 255;
  
  desaloca(a);
  
  a = (int *)aloca(sizeof(int)*10);
  *a = 120;
  
  int *b = (int *)aloca(sizeof(int)*10);
  
  *b = 600;
  
  int *c = (int *)aloca(sizeof(int)*3);
  
  *c = 255;
  
  //desaloca(a);
  //a = (int *)aloca(sizeof(int)*20); //Segment Fault!!!
  
  printf("%d, %d, %d\n\n", *a, *b, *c);
  
  desaloca(a);
  desaloca(b);
  desaloca(c);
  
  int i;
  for(i=0; i<ALLOCSIZE; i++) {
    printf("[%d]", buf[i]);
  }
  printf("\n");
  
  return 0;
}

//Desaloca um espaço do buffer, apenas setando o indicador de espaço oculpado 1 para 0
void desaloca(void *p) {
  byte *pAux = (byte *)p - 3;
  *pAux = 0;
}

//Aloca um espaço no buffer e retorna um ponteiro para o mesmo
void *aloca(int n) {
  int i = 0;
  
  while(i < ALLOCSIZE-3) { //Percorre o buffer até seu tamanho máximo descontando os 3 bytes usados para controle
    if(buf[i] == 1 && buf[i+1] > 0 && buf[i+2] == (1 + buf[i+1])) //Verifica se determinado espaço do buffer está oculpado e desloca o número de passos necessários
      i = i + buf[i+1] + 3;
    else
      if(verificaBufEspaco(i, n + 3) == 1)
	break;
      else
	i++;
  }
  //printf("{%d} %d\n", (ALLOCSIZE -3 - i), n);
  if((ALLOCSIZE - 3 - i) < n)
    return 0;
  
  //Seta as variáveis de controle
  buf[i] = 1;
  buf[i+1] = n;
  buf[i+2] = n + 1; //Váriavel confirmadora de espaço utilizado válido!
  
  return &buf[i+3];
}

//Verifica se existe espaço suficiente no buffer para "encaixar" a quantidade de bytes n
int verificaBufEspaco(int i, int n) {
  int iAux = i;
  
  while(iAux < (n + i) && i < ALLOCSIZE-3) {
    //printf("%d == 1, %d > 0, {%d == %d}\n", buf[i], buf[i+2], buf[i+2], (1 + buf[i+1]));
    if(buf[iAux] == 1 && buf[iAux+1] > 0 && buf[iAux+2] == (1 + buf[iAux+1])) //Caso no espaço solicitado exista um espaço que já está sendo utilizado, a função retorna 0
      return 0;
    iAux++;
  }
  
  //Caso haja espaço suficiente para alocar a quantidade n de bytes requisitados, a função retorna 1
  return 1;
}