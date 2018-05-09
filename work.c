#include <unistd.h>     /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <signal.h>

// awk -v data="$(<a.txt)" '/>>>/ {f=1} /<<</ && f {print data; f=0}1' teste3.txt > file.txt


/*

The exec() functions return only if an error has occurred. 
The return value is -1, and errno is set to indicate the error.  

USAR EXECVP na encadeia para usar pipes em que os argumentos sao:
EXEMPLO: execvp("ls", ["ls", "-la", (char*) NULL]);


*/

int my_system(char* cmd) {
  int c = 0,i;
  int fd[2];
      for (int t = 0; t < strlen(cmd); t++) {
        if(cmd[t] == '|') c++;
      }
      c++;
      
      char* pch;
      char* result[c];
      int j = 0;
      pch = strtok (cmd,"|");
      while (pch != NULL){
        result[j] = strdup(pch);
        j++;
        pch = strtok (NULL, "|");
      }
  
      for(i = 0; i < c-1 ; i++) {
    
        pipe(fd); // cria um pipe com o par fd
        char* ar[5];
        int z = 0;
        pch = strtok (result[i]," ");
        while (pch != NULL){
          ar[z] = strdup(pch);
          z++; 
          pch = strtok (NULL, " ");
        }
        
        while(z < 3) {
          ar[z] = (char*) 0;
          z++;
        }
    
        if(!fork()) { // Se for um filho
          close(fd[0]); // fecha o file descriptor 0
          dup2(fd[1],1); // direciona o conteúdo de fd[1] para o stdoutput
          close(fd[1]); // fecha o 1
          execvp(ar[0], ar); // executa o comando em argv[i]
          _exit(1); // sai
        }
        dup2(fd[0],0); // direciona o conteúdo de fd[0] para o stdinput
        close(fd[0]); // fecha 0
        close(fd[1]); // fecha 1
    }
    
    char* ar[3];
    ar[0] = (char*) 0;
    ar[1] = (char*) 0;
    ar[2] = (char*) 0;
    pch = strtok (result[i]," ");
    j = 0;
    while (pch != NULL){
      ar[j] = strdup(pch);
      j++;
      pch = strtok (NULL, " ");
    }

    // teste
    int out = open("cout.log", O_RDWR | O_CREAT | O_APPEND, 0600);
    if (-1 == out) { perror("opening cout.log"); return 255; }

    int err = open("cerr.log", O_RDWR|O_CREAT|O_APPEND, 0600);
    if (-1 == err) { perror("opening cerr.log"); return 255; }

    int save_out = dup(fileno(stdout));
    int save_err = dup(fileno(stderr));

    if (-1 == dup2(out, fileno(stdout))) { perror("cannot redirect stdout"); return 255; }
    if (-1 == dup2(err, fileno(stderr))) { perror("cannot redirect stderr"); return 255; }

    // teste
    pid_t pid;
    if ((pid = fork()) == 0) {
      execvp(ar[0], ar); // pai executa o último
      _exit(1);
    }
    waitpid(pid, NULL, 0);

    fflush(stdout); close(out);
    fflush(stderr); close(err);

    dup2(save_out, fileno(stdout));
    dup2(save_err, fileno(stderr));

    close(save_out);
    close(save_err);

    return 0;
}


#define BUF_SIZE 1024

ssize_t readline(int fildes, void *buf, size_t nbyte) {
    ssize_t nb = 0;
    char* p = buf;
    while (1) {
        if (nb >= nbyte) break;
        int n = read(fildes, p, 1);
        if (n <= 0) break;
        nb += 1;
        if (*p == '\n') break;
        p += 1;
    }
    return nb;
}

char* comando(char* s) {
	char *aux = malloc(strlen(s)+1);
	char* pch;
	int i = 0;
	pch = strtok (s," ");
	if(strcmp(pch,"$") == 0 || strcmp(pch,"$|") == 0) {
  		while (pch != NULL){
  			if (i == 1) strcpy(aux, pch); 
  			if (i > 1 && pch[0] == '-') {
  				strcat(aux, " ");
  				strcat(aux, pch);  			 
        }
  			i++;
    		pch = strtok (NULL, " ");
  		}
  	}
  	return aux;
}

void filtra(char* s) {
  int i = 0;
  while(s[i] != '\0') {
    if(s[i] == '\n') s[i] = ' ';
    i++;
  }
}

// usar Lista Ligada de comandos
int main(int argc, char* argv[]) {
	int fd[2];
  
  int fd_1 = open(argv[1], O_RDWR);
  if (-1 == fd_1) { perror("opening argv[1]"); return 254; }

  int out = open("cout.log", O_RDWR | O_CREAT | O_APPEND, 0600);
  if (-1 == out) { perror("opening cout.log"); return 255; }

	char buf[BUF_SIZE] = "";
	int nl = 1;
	char* result[3]; // tem que ser lista ligada de char*
	int i = 0;
  char cmd[100];
  
	while(1) {
		size_t n = readline(fd_1, buf,sizeof(buf));
		if (n <= 0) break;
    //write(1, buf, n);
		write(out, buf, n);
    
    if (buf[0] == '$') {
      result[i] = strdup(comando(buf));
      for(int z = 0; z < strlen(result[i]); z++) {
        if(result[i][z] == '\n') result[i][z] = ' ';
      }
			i++;

      for (int j = 0; j < i; j++) {
        if (j == 0) strcpy(cmd, result[j]);
        else {
          strcat(cmd, " | "); 
          strcat(cmd, result[j]); 
        }
      }
      write(out, ">>>\n", 4);
      pid_t pid;
      if ((pid = fork()) == 0) {
        int x = my_system(cmd);
        write(out, "<<<\n", 4);
      }
      waitpid(pid, NULL, 0);        
		}
		strcpy(buf,"                     \0");
	}
	close(fd_1);
  	close(out);
	
	return 0;
}
