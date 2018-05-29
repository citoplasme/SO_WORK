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

/*

The exec() functions return only if an error has occurred. 
The return value is -1, and errno is set to indicate the error.  

USAR EXECVP na encadeia para usar pipes em que os argumentos sao:
EXEMPLO: execvp("ls", ["ls", "-la", (char*) NULL]);

*/

/* 
  Função que encadeia os comandos dados no input usando pipes
  Input é do tipo "cat | wc -l"
  Contéudo do output é escrito no cout.log
  Conteúdo de erro no cerr.log
*/
int encadear(char* cmd) {
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
    
        int aux = pipe(fd); // cria um pipe com o par fd
        if (aux == -1) {
        	perror("Creating a pipe.");
        	return 333;
        }
        char* ar[10];
        int z = 0;
        pch = strtok (result[i]," ");
        while (pch != NULL){
          	ar[z] = strdup(pch);
          	z++; 
         	pch = strtok (NULL, " ");
        }
        
        while(z < 10) {
          	ar[z] = (char*) 0;
          	z++;
        }
    
        if(!fork()) { // Se for um filho
          	close(fd[0]); // fecha o file descriptor 0
          	int dupaux = dup2(fd[1],1); // direciona o conteúdo de fd[1] para o stdoutput
          	if (dupaux == -1) {
        		perror("Dup failed.");
        		return 334;
        	}
          	close(fd[1]); // fecha o 1
          	int auxexec = execvp(ar[0], ar); // executa o comando em argv[i]
          	if (auxexec == -1) {
        		perror("Exec failed.");
        		return 335;
        	}
          	_exit(1); // sai
        }
        int auxdup2 = dup2(fd[0],0); // direciona o conteúdo de fd[0] para o stdinput
        if (auxdup2 == -1) {
        	perror("Dup failed.");
        	return 336;
        }
        close(fd[0]); // fecha 0
        close(fd[1]); // fecha 1
    }
    
    char* ar[10];
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

    int out = open("cout.log", O_RDWR | O_CREAT | O_APPEND, 0600);
    if (-1 == out) { 
      	perror("opening cout.log"); 
      	return 255; 
    }

    int err = open("cerr.log", O_RDWR|O_CREAT|O_APPEND, 0600);
    if (-1 == err) { 
      	perror("opening cerr.log"); 
      	return 255; 
    }

    int save_out = dup(fileno(stdout));
    int save_err = dup(fileno(stderr));

    if (-1 == dup2(out, fileno(stdout))) { 
      	perror("cannot redirect stdout"); 
      	return 255; 
    }

    if (-1 == dup2(err, fileno(stderr))) { 
      	perror("cannot redirect stderr"); 
      	return 255; 
    }

    pid_t pid;
    if ((pid = fork()) == 0) {
      	int auxexec = execvp(ar[0], ar); // pai executa o último
      	if (auxexec == -1) {
        	perror("Exec failed.");
        	return 335;
        }
      	_exit(1);
    }
    waitpid(pid, NULL, 0);

    fflush(stdout); close(out);
    fflush(stderr); close(err);

    int auxdup = dup2(save_out, fileno(stdout));
    if (auxdup == -1) {
       	perror("Dup failed.");
       	return 334;
    }
    int auxdup2 = dup2(save_err, fileno(stderr));
	if (auxdup2 == -1) {
       	perror("Dup failed.");
       	return 334;
    }

    close(save_out);
    close(save_err);

    return 0;
}


#define BUF_SIZE 1024

/*
  Função que lê uma linha de um ficheiro
  Devolve a quantidade de bytes lidos
*/
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

/*
  Função que serve para ler um comando de uma linha do ficheiro
*/
char* comando(char* s) {
  	char *aux = malloc(strlen(s)+1);
	char* pch;
	int i = 0;
	pch = strtok (s," ");
	if(strcmp(pch,"$") == 0 || strcmp(pch,"$|") == 0) {
    	while (pch != NULL){
  			if (i == 1) strcpy(aux, pch); 
  			if (i > 1 /*&& pch[0] == '-'*/) { // ver este caso
  				strcat(aux, " ");
  				strcat(aux, pch);  			 
        }
  			i++;
    		pch = strtok (NULL, " ");
  		}
  	}
    return aux;
}


int cpy_file(char* f1, char* f2) {
  	int x = open(f1, O_RDONLY);
  	if (-1 == x) { 
    	perror("opening f1"); 
    	return 255; 
 	}

  	int y = open(f2, O_WRONLY | O_TRUNC);
  	if (-1 == y) { 
    	perror("opening f2"); 
    	return 256; 
  	}

	int err = open("cerr.log", O_RDWR | O_CREAT | O_APPEND, 0600);
    if (-1 == err) { 
      	perror("opening cerr.log"); 
      	return 255; 
    }

    int save_out = dup(fileno(stdout));
    int save_err = dup(fileno(stderr));


    if (-1 == dup2(y, fileno(stdout))) { 
      	perror("cannot redirect stdout"); 
      	return 255; 
    }

    if (-1 == dup2(err, fileno(stderr))) { 
      	perror("cannot redirect stderr"); 
      	return 255; 
    }

  	pid_t pid;
  	char* list[] = {"cat", f1, (char*) NULL};
  	if ((pid = fork()) == 0) {
    	execvp(list[0], list);
    	_exit(1);
  	}

    waitpid(pid, NULL, 0);
    
    close(x); 
    close(y);
    close(err);

    dup2(save_out, fileno(stdout));
    dup2(save_err, fileno(stderr));

    close(save_out);
    close(save_err);

  	return 0;
}

void interrupt(int i){
  	int status;
  	int filho = wait(&status);
  	int err = open("cerr.log", O_WRONLY | O_CREAT | O_APPEND, 0600);

  	if (-1 == err) {
  		perror("CTRL C pressed"); 
  		return;
  	} 
  	write(err, "CTRL C\n", sizeof("CTRL C\n"));
  	close(err);

  	exit(0);
}


int main(int argc, char* argv[]) {
	int fd[2];
  
  	int fd_1 = open(argv[1], O_RDWR);
  	if (-1 == fd_1) { 
    	perror("opening argv[1]"); 
    	return 254; 
  	}

 	int out = open("cout.log", O_RDWR | O_CREAT | O_APPEND, 0600);
  	if (-1 == out) { 
    	perror("opening cout.log"); 
    	return 255; 
  	}

	char buf[BUF_SIZE] = "";
	char buffer[BUF_SIZE] = "";
  	int nl = 1;
	char* result[10]; 

	int i = 0;
  	char cmd[100];
 
 	signal(SIGINT, interrupt);
  
	while(1) {
		size_t n = readline(fd_1, buf,sizeof(buf));
		if (n <= 0) break;
    
    	if (buf[0] == '>' && buf[1] == '>' && buf[2] == '>') nl = 0;
    	if (buf[0] == '<' && buf[1] == '<' && buf[2] == '<') {nl = 1; continue;}    
    	if (nl == 0) continue;
    	else {
    		write(out, buf, n);
    		if (buf[0] == '$') {
      			result[i] = strdup(comando(buf));
      			for(int z = 0; z < strlen(result[i]); z++) {
        			if(result[i][z] == '\n') result[i][z] = ' ';
      			}
				i++;

				if (buf[1] == '|') { // só ir até ao último com $ !!!!!
					for (int j = 0; j < i; j++) {
						if (j == 0) strcpy(cmd, result[j]);
    			    	else {
    			      		strcat(cmd, " | "); 
    			      		strcat(cmd, result[j]); 
    			    	}
    			  	}
  				}
  				else strcpy(cmd, result[i-1]);
  				
  				write(out, ">>>\n", 4);
      			pid_t pid;
      			if ((pid = fork()) == 0) {
        			int x = encadear(cmd);
        			if (x != 0) {
        				perror("Encadeia Failed");
        			}
        			write(out, "<<<\n", 4);
        			_exit(1);
      			}
      		waitpid(pid, NULL, 0);        
			}
  		}
		strcpy(buf,"                     \0");
	}
	close(fd_1);
  	close(out);

  	// copiar ficheiro para o input
  	int err = open("cerr.log", O_RDWR | O_CREAT | O_APPEND, 0600);
    if (-1 == err) { 
      	perror("opening cerr.log"); 
      	return 255; 
    }
    int ler = read(err,buffer, sizeof(buffer));
    close(err);

    if (ler == 0) {
  		int x = cpy_file("cout.log", argv[1]);
		if (x != 0) { 
			perror("Update failed."); 
			return 266; 
		}
	}

	pid_t pid1;
    if ((pid1 = fork()) == 0) {
        char* rmErr[] = {"rm", "cerr.log", (char*) NULL};
        execvp(rmErr[0],rmErr);
        _exit(1);
    }
    waitpid(pid1, NULL, 0);  

	pid_t pid2;
    
    if ((pid2 = fork()) == 0) {
        char* rmOut[] = {"rm", "cout.log", (char*) NULL};
        execvp(rmOut[0],rmOut);
        _exit(1);
    }
    waitpid(pid2, NULL, 0);  
	
	return 0;
}
