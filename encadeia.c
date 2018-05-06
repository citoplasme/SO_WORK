#include <unistd.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int fd[2];
	int i;
	// Retira os comandos
	char cmd[100] = "";
	for (int j = 1; j < argc; ) {
		if (j == argc -1) {
			strcat(cmd, argv[j]);	
			j++;
		}

		else if (argv[j + 1][0] == '-' && j == argc -2) {
			strcat(cmd, argv[j]);
			strcat(cmd, " ");
			strcat(cmd, argv[j+1]);
			j+=2;
		}

		else if (argv[j + 1][0] == '-') {
			strcat(cmd, argv[j]);
			strcat(cmd, " ");
			strcat(cmd, argv[j+1]);
			strcat(cmd, " | ");
			j+=2;
		}

			else {
				strcat(cmd, argv[j]);
				strcat(cmd, " | ");
				j++;
			}
	}
	//printf("%s\n",cmd );
	// contar qts comandos ha
	int c = 0;
	for (int t = 0; t < strlen(cmd); t++) {
		if(cmd[t] == '|') c++;
	}
	c++;
	//printf("%d\n",c );
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
		char* ar[3];
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
	execvp(ar[0], ar); // pai executa o último
	
	return 0;	
}
