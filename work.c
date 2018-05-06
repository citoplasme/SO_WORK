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




#define BUF_SIZE 1024

int my_system(const char *command) {
    sigset_t blockMask, origMask;
    struct sigaction saIgnore, saOrigQuit, saOrigInt, saDefault;
    pid_t childPid;
    int status, savedErrno;

    if (command == NULL)                /* Is a shell available? */
        return my_system(":") == 0;

    /* The parent process (the caller of system()) blocks SIGCHLD
       and ignore SIGINT and SIGQUIT while the child is executing.
       We must change the signal settings prior to forking, to avoid
       possible race conditions. This means that we must undo the
       effects of the following in the child after fork(). */

    sigemptyset(&blockMask);            /* Block SIGCHLD */
    sigaddset(&blockMask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blockMask, &origMask);

    saIgnore.sa_handler = SIG_IGN;      /* Ignore SIGINT and SIGQUIT */
    saIgnore.sa_flags = 0;
    sigemptyset(&saIgnore.sa_mask);
    sigaction(SIGINT, &saIgnore, &saOrigInt);
    sigaction(SIGQUIT, &saIgnore, &saOrigQuit);

    switch (childPid = fork()) {
    case -1: /* fork() failed */
        status = -1;
        break;          /* Carry on to reset signal attributes */

    case 0: /* Child: exec command */

        /* We ignore possible error returns because the only specified error
           is for a failed exec(), and because errors in these calls can't
           affect the caller of system() (which is a separate process) */

        saDefault.sa_handler = SIG_DFL;
        saDefault.sa_flags = 0;
        sigemptyset(&saDefault.sa_mask);

        if (saOrigInt.sa_handler != SIG_IGN)
            sigaction(SIGINT, &saDefault, NULL);
        if (saOrigQuit.sa_handler != SIG_IGN)
            sigaction(SIGQUIT, &saDefault, NULL);

        sigprocmask(SIG_SETMASK, &origMask, NULL);

        execl("/bin/sh", "sh", "-c", command, (char *) NULL);
        _exit(127);                     /* We could not exec the shell */

    default: /* Parent: wait for our child to terminate */

        /* We must use waitpid() for this task; using wait() could inadvertently
           collect the status of one of the caller's other children */

        while (waitpid(childPid, &status, 0) == -1) {
            if (errno != EINTR) {       /* Error other than EINTR */
                status = -1;
                break;                  /* So exit loop */
            }
        }
        break;
    }

    /* Unblock SIGCHLD, restore dispositions of SIGINT and SIGQUIT */

    savedErrno = errno;                 /* The following may change 'errno' */

    sigprocmask(SIG_SETMASK, &origMask, NULL);
    sigaction(SIGINT, &saOrigInt, NULL);
    sigaction(SIGQUIT, &saOrigQuit, NULL);

    errno = savedErrno;

    return status;
}

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

//int executa(char* comando[]);

void filtra(char* s) {
  int i = 0;
  while(s[i] != '\0') {
    if(s[i] == '\n') s[i] = ' ';
    i++;
  }
}

// usar Lista Ligada de comandos
int main(int argc, char* argv[]) {
	int fd = open(argv[1], O_RDWR);
	char buf[BUF_SIZE] = "";
	int nl = 1;
	char* result[3]; // tem que ser lista ligada de char*
	int i = 0;
  char cmd[100];
  //int fd_aux = open("aux.txt", O_CREAT | O_WRONLY);
	
	while(1) {
		size_t n = readline(fd, buf,sizeof(buf));
		if (n <= 0) break;
    write(1, buf, n);
		if (buf[0] == '$') {
      result[i] = strdup(comando(buf));
      for(int z = 0; z < strlen(result[i]); z++) {
        if(result[i][z] == '\n') result[i][z] = ' ';
      }
			i++;

      for (int j = 0; j < i; j++) {
        //printf("-> %s <- ",result[j] );
        if (j == 0) strcpy(cmd, result[j]);
        else {
          strcat(cmd, " | "); 
          strcat(cmd, result[j]); 
        }
      }
      
      //printf("\n COMANDO: %s\n", cmd);
      //printf("%d\n",i);
      write(1, ">>>\n", 4);
      
      pid_t childPid;
      if ((childPid = fork()) == 0) {
        //close(fd);
        //dup2(1, fd_aux);
        //close(fd_aux);
        execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
        _exit(1);
      }

      int status, savedErrno;

      while (waitpid(childPid, &status, 0) == -1) {
            if (errno != EINTR) {       /* Error other than EINTR */
                status = -1;
                break;                  /* So exit loop */
            }
        }
        write(1, "<<<\n", 4);
		}
		strcpy(buf,"                     \0");

      
	}

	close(fd);
	//execlp("./encadeia", result[0], result[1], result[2]);
	/*printf("0 -> %s\n",result[0] );
	printf("1 -> %s\n",result[1] );
	printf("2 -> %s\n",result[2] );*/
	//execlp("./encadeia", "./encadeia","ls", "sort", "wc", NULL);
	/*char* cmds = malloc (strlen(result[0]) + strlen(result[1]) + strlen(result[2]) + 8);
	strcpy(cmds, result[0]);
	strcat(cmds, " | ");
	strcat(cmds, result[1]);
	strcat(cmds, " | ");
	strcat(cmds, result[2]);
	for(int i = 0; i < strlen(cmds); i++) {
		if(cmds[i] == '\n') cmds[i] = ' ';
	}
	printf("%s\n",cmds );
	//my_system(cmds);
  pid_t childPid;
  if ((childPid = fork()) == 0) {
    execl("/bin/sh", "sh", "-c", cmds, (char *) NULL);
    _exit(1);
  }

  int status, savedErrno;

  while (waitpid(childPid, &status, 0) == -1) {
            if (errno != EINTR) {       
                status = -1;
                break;                  
            }
        }
*/
	
	
	return 0;
}
