/******************************************************************
 ********PRACTICA: MINISHELL BÁSICO *******************************                          
 ******  AUTORES: Serra Rigo, Sebastià ****************************
 *************    Pérez Joan, Miquel ****************************** 
 ******************************************************************/

#define _POSIX_SOURCE
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> // for close
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "shell.h"

#define MAX 100
#define MAX_ARGS 255
#define EXIT -1
#define MAX_BCKG 1024


pid_t foreground_pid = 0;
pid_t bckg_processes[MAX_BCKG];
pid_t num_bckg = 0;


/******************************************************************
 *******************  FUNCION: separarCommands()  *****************                          
 ******  Parametros: -(char **) array de argumentos ***************
 ******************  -(char *) str: linea de la que va a separar **
 ******************  -(char *) s: token ***************************
 ******	 Return: (void)  ****************************************** 
 ******************************************************************/

void separarCommands(char **args, char * str, char *s){
	*args = strtok(str, s); //str por parametros para que recorra hasta el primer espacio
	while(*args){
		args++;
     	 	*args = strtok(NULL, s); //NULL por parametros para que siga recorriendo desde
	}				//donde quedó el puntero la ultima vez que se hizo strtok()
	args++;
 	*args=NULL;  //ultimo argumento a NULL
}


/*****************************************************************
 *******************  FUNCION: isBackground()  *******************                          
 ******  Parametros: -(char **) array de argumentos **************
 ******	 Return: 1 si encuentra '&', 0 si no  ******************** 
 *****************************************************************/

int isBackground(char **args){ //Comprueba si el último argumento es '&'
	while(*args){ //recorremos args
	    args++;
	}
	args--; //situamos el puntero en el último argumento
	
	if(! strcmp(*args, "&")){ //miramos si e '&'
		
		return 1;
	}
	return 0; 
}

/*****************************************************************
 *******************  FUNCION: removeBckg()   ********************                          
 ******  Parametros: -(int) PID del proceso a eliminar del *******
 ******************  array de PIDs en background  ****************
 ******	 Return: (int) 1 si lo ha eliminado, 0 si no**** ********* 
 *****************************************************************/

int removeBckg(int bckg){
	int i = 0;
	for(;i < num_bckg; i++){
		if(bckg_processes[i] == bckg){
		      bckg_processes[i] = bckg_processes[num_bckg-1];
		      num_bckg--;
		      return 1;
		}
	}
	return 0;
	
	
}


/*****************************************************************
 *******************  FUNCIÓN: isInternal()  *********************                          
 ******  Parametros: -(char *) primer argumento  *****************
 ******	 Return: (int) 1 si es cualquier comando interno, ******** 
 *******************  0 si no es interno y -1 si es 'exit'  ******
 *****************************************************************/

int isInternal(char *arg){ //tru o fals
	if(!strcmp(arg,"cd")) return 1;
	if(!strcmp(arg,"source")) return 1;
	if(!strcmp(arg,"jobs")) return 1;
	if(!strcmp(arg, "export")) return 1;
	if(!strcmp(arg, "exit")) return -1;
	return 0;
}

/*****************************************************************
 *******************  FUNCIÓN: execInternal()   ******************                          
 ******  Parametros: -(char **) array de argumentos  *************
 ******	 Return: (void)   ****************************************
 *****************************************************************/

void execInternal(char ** args){
 
    if(!strcmp(*args,"cd")){ //CD
          
      if(chdir(args[1])) printf("No existe el archivo o directorio\n"); //si existe el directorio, lo cambiamos
    
    }else if(!strcmp(*args,"source")){ //SOURCE 
	
	char *line;
	FILE* file = fopen(args[1], "r"); //archivo del que leeremos comandos
	
	do{
		line = malloc(1024);	
		execute_line(line); //ejecutamos la línea leída en el fichero

       	}while(fgets(line, 500, file)); //va leyendo las líneas hasta que encuentra NULL al final del fichero.


    }else if(!strcmp(*args,"jobs")){ //JOBS
	  int i = 0;			//enseña el array de PIDs en background
	  for(; i<num_bckg; i++) printf("[%i]\tPID: %i\n", i+1, bckg_processes[i]);

    }else if(!strcmp(*args, "export")){ //EXPORT 
      					//modifica variables de entorno
      char * a = strtok(args[1], "=\n");
      
      char * b = a + strlen(a)+1;
	
      setenv(a, b, 1);
      
    }
  
}


/*****************************************************************
 *******************  FUNCIóN: ctrlc()   *************************                          
 ******  Parametros: (int) número de la señal  *******************
 ******	 Return: (void)  ***************************************** 
 *****************************************************************/

void ctrlc(int signum){


	signal(SIGINT, ctrlc);
	if(foreground_pid > 0){
        	kill(SIGINT, foreground_pid);
	}else puts("Nope");
}

/*****************************************************************
 *******************  FUNCION: nozombie()   **********************                          
 ******  Parametros: (int) número de la señal  *******************
 ******	 Return: (void)  ***************************************** 
 *****************************************************************/

void nozombie(int signum){
	signal(SIGCHLD, nozombie);
    
	int kidpid = waitpid(-1, NULL, WNOHANG);
	if(kidpid != -1 && removeBckg(kidpid)) printf("Ha acabado la tarea con pid %i \n", kidpid); //cuando acaba el proceso en background
}

/*****************************************************************
 *******************  FUNCION: execute_line()   ******************                          
 ******  Parametros: array de chars (línea) a tratar  ************
 ******	 Return: (int) 1 si es interno, 0 si es externo, ********* 
 *******************  y -1 si es 'exit'  *************************
 *****************************************************************/

int execute_line(char* line){
	
	char *s = " \n";
	char *args[MAX_ARGS];
	pid_t pid;
	int status;
	int command = 0;
		  
	  separarCommands(args, line, s); //separamos line por comandos
	  
	 
	  if(*args){ //si line tiene argumentos
		command = isInternal(args[0]);
		if(command){ //si es comando interno
		      
		      execInternal(args);
		}else{ //si no es comando interno
		  
		      pid = fork();
		      
		      if(!pid){
			      if( isBackground(args) ){ //miramos si es background
				      foreground_pid = 0; 
				      signal(SIGINT, SIG_IGN);
				      char ** prev =(char**) args;
				      while(*prev) prev++;
				      prev--;
				      *prev = NULL;
			      }
			      if( numberOfArgs(args)  >= 3){ //como mínimo habra "(comando1) > (fichero)", es decir, 3 argumentos
				      if(!strcmp(args[numberOfArgs(args) -2], ">")){ //si el penultimo argumento es '>'
					      int lastArg = numberOfArgs(args) - 1;
					      int fd = open(args[lastArg], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
					      close(1);
					      dup(fd); //cambia STDOUT por el file descriptor 'fd'
					      args[numberOfArgs(args) -2] = NULL;
				      }
			      }
			    execvp(args[0], args); //ejecuta el comando externo (y sus argumentos)
			    printf("Falló el exec de %s\n", args[0]);
			    return -1; //así el programa principal saldrá al pensar que se ha escrito un exit 			
		      }
		      
		      if(isBackground(args)){ //si es background
			      
			      bckg_processes[num_bckg] = pid;
			      num_bckg++;
			      foreground_pid = 0;
		      }else{ //si no es background
			      
			      foreground_pid = pid;
			     
			      wait(&status);
		      }


		}
	  }
	  free(line); //readline ha hecho un malloc() de line previamente, liberamos la memoria correspondiente
	
     return command;
  	
}

/*****************************************************************
 *******************  FUNCIÓN: numberOfArgs()   ******************                          
 ******  Parametros: array de arrays de char (char ** args)  *****
 ******	 Return: (int) numero de elementos de 'args'. ************
 *****************************************************************/

int numberOfArgs(char **args){ 
	char ** inicial = args;
	while(*args) args++;
	return args - inicial;
}

/*****************************************************************
 **************************  MAIN  *******************************
 *****************************************************************/

int main()
{
   
	printf("\e[1;34m************************************\n");
	printf("*          MINISHELL v1.0          *\n");
	printf("************************************\e[0m\n\n");
	
	signal(SIGINT, ctrlc);
	
	signal(SIGCHLD, nozombie); 	
	
	char cwd[1024];	
	char *line;
	
	int command = 0;

	while(command != EXIT){ //mientras el comando introducido no sea 'exit'

	  getcwd(cwd, sizeof(cwd)); //obtenemos nuestro CWD
	  char *user = getenv("USER"); // obtenemos nuestro USER desde las variables de entorno
	  printf("\e[1;32m%s\e[0m:", user); //mostramos por pantalla parte del prompt (USER)
	  line = readline(strcat(cwd, "\033[5m$\e[0m ")); //mostramos por pantalla pa parte restante del prompt
	  if(strcmp(line,"")) add_history(line); //si la línea no está vacía, la añade al historial
	
	  command = execute_line(line); //ejecuta la línea y obtiene el tipo de comando que ha sido (external, internal, 
					//o exit para finalizar)

	} 
	
	return(0);
}
