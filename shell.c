#include <stdlib.h>   
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 100
#define MAX_N_PARAMS 10
#define TRUE 1
#define FALSE 0
#define DEFAULT 1

struct process{
	pid_t _pid;
	int prioridad;
	struct process *next;
};

int parse(char* input, char** params) { //Dividimos el input en un arreglo de parametros
	int i;
    for(i=0; i<MAX_N_PARAMS; i++) {
        params[i] = strsep(&input, " ");//Se guardan los comandos separados por espacio en el arreglo de paramteros
        if(params[i] == NULL) break;

    }
    return i;
};

int execute(char** params, char* log, int background) {
    pid_t pid = fork(); //Fork

    if (pid == -1) { //Error
        printf("Error haciendo fork!!\n");
        return 1;
    } else if (pid == 0) { // Proceso hijo
        execvp(params[0], params);

        /* En el caso que falle el execvp */
        printf("Comando Desconocido\n");
        _exit(127); //Este exit es necesario para saber si fallo el execvp, Exit de command not found
        return 1;

    } else if (!background){ // Proceso padre
        int status;
        int wait = waitpid(pid, &status, 0);
        if (wait > 0) {
            if (WIFEXITED(status) && !WEXITSTATUS(status)) { //Si se cumple el execvp termino bien
                return 0;
            } else { //El excecvp termino mal
                return 1;
            }
        }
        return 1;
    } else if(background){
    	printf("[ ] %i\n",pid);
    	int status;
        int wait = waitpid(pid, &status, WNOHANG);
        if (wait > 0) {
            if (WIFEXITED(status) && !WEXITSTATUS(status)) { //Si se cumple el execvp termino bien
                return 0;
            } else { //El excecvp termino mal
                return 1;
            }
        }
        return 1;
    }
};

void printList(struct process *node){
	while(node != NULL){
		printf("[%i] \n",node->_pid);
		node = node->next;
	}
}

int main() {    
    char input[MAX_INPUT_LENGTH+1];    
    char * params[MAX_N_PARAMS+1];

    /* Cabeza Lista procesos background */
    /* En esta cola se va agregando desde la cabeza. Ejemplo, queremos agregar (x) a (Cabeza)--(obj1)--(obj2)*/
    /* Resultado: (Cabeza)--(x)--(obj1)--(obj2) */
    struct process *childrenBackground = (struct process*)malloc(sizeof(struct process));
    childrenBackground->_pid = -1;
    childrenBackground->prioridad = -1;
    childrenBackground->next = NULL;


    int currentSize = 1;
    char *success = " Success\n\0"; //Mensaje de comando satisfactorio en el log
    char *failed = " Failed\n\0"; //Mensaje de comando erroneo en el log
    char *log = malloc(sizeof(char)); //Creamos el log, el cual es un string dinamico
    int cantidadParametros = 0;
    int background = FALSE;
    int exitCondition = FALSE; 

  	/* Open log file */
    FILE *fptr; //Puntero a tipo archivo
    fptr = fopen("mishell.log","a"); //Abrimos el archivo en modo append (para escribir sobre lo que existe), de no existir es creado

    if (fptr == NULL)
    { 
        printf("Error abriendo el archivo log"); 
        return 0; 
    } 

    while(!exitCondition) {

    	/* Revisar estado procesos en el background */
    	int bgreturn = 0;
    	struct process *current = childrenBackground;
    	while(current->next != NULL){

    		/* Este waitpid no bloquea la ejecucion  */
    		if(waitpid(current->next->_pid, &bgreturn, WNOHANG) == current->next->_pid){ // retonar true si un proceso de la lista hizo exit
    			printf("[%i] exited with status %i \n",current->next->_pid, bgreturn); 

    			if (WIFEXITED(bgreturn) && !WEXITSTATUS(bgreturn)) { //Si se cumple el execvp termino bien
	                currentSize += strlen(success);//Aumentamos el tamano del string log segun el largo del string success
		            log = realloc(log, currentSize * sizeof(char));
		            strcat(log,success);
	            }else{ //El excecvp termino mal
	                currentSize += strlen(failed);//Aumentamos el tamano del string log segun el largo del string failed
		            log = realloc(log, currentSize * sizeof(char));
		            strcat(log,failed);
	            }

	            /* Elimina el proceso de la lista (childrenBackground)*/
	            struct process *tmp = current->next;
	            current->next = tmp->next;
	            tmp = NULL;
	            free(tmp);
    		}
    		current = current->next;
    		if(current == NULL) break;
    	}
    	
    	/* Imprime la lista de proceso en el background (Testing) */
    	// printList(childrenBackground);
    	
    	/* Prompt */
        printf("CustomUnixShell$:");

        /* Input vacio con CTRL+D cierra la shell (???)*/
        if(fgets(input, sizeof(input), stdin) == NULL) break; 

        /* Caso enter */
        if(input[0] == '\n') continue; //Si se presiona enter sin un comando previo, imprime de nuevo la shell

        /* Se remueve el caracter de salto de linea al final del input */
        if(input[strlen(input)-1] == '\n') { 
            input[strlen(input)-1] = '\0';    
        }

        /* Realizar log de comandos */
        currentSize += strlen(input); //Aumentamos el tamano del string log segun el largo del input
        log = realloc(log, currentSize * sizeof(char));
        strcat(log, input); // Concatenamos el input al log
        cantidadParametros = parse(input, params); //Dividimos el input en un arreglo de parametros  
        if (!strcmp(params[0], "log")){ //Se crea el archivo de log con los datos recopilados 
            fprintf(fptr,"%s\n",log); //Guardamos el log en el archivo
            continue;
        }     

        /* Caso exit */
        if (strcmp(params[0], "exit") == 0){
        	if(childrenBackground->next != NULL){
        		printf("Existen procesos ejecutandose en el background. \n");
        		printList(childrenBackground);
        		printf("Deseas salir de todas formas ? (y) / (n) \n");	
        		char c = getchar();
        		if(c == 121 || c == 89){
        			exitCondition = TRUE;
        			break;
        		} 
        		getchar(); // Evita un print de prompt  innecesario
        		continue;

        	} else {
        		exitCondition = TRUE;
        		break;
        	}
        }

        /* Ver si debe ejecutar en background */
        if(strcmp(params[cantidadParametros - 1], "&") == 0){
        	params[cantidadParametros - 1] = '\0'; // Se borra el &
        	background = TRUE;
        }

        /* Ejecutar comandos */
        if(cantidadParametros > 0){
        	pid_t pid = fork();

        	if (pid < 0){ // Error
	        		printf("No se pudo iniciar proceso\n");
	        		exit(1); 

	        } else if (pid == 0){
	        		execvp(params[0], params);

	        		/* En el caso que falle el execvp */
			        printf("Comando Desconocido\n");
			        _exit(127); //Este exit es necesario para saber si fallo el execvp, Exit de command not found

	        } else {
	        	if (!background){ // Espera por el proceso
	        		int status; 
	        		int wait = waitpid(pid, &status, 0);
	        		if (wait > 0) {
			            if (WIFEXITED(status) && !WEXITSTATUS(status)) { //Si se cumple el execvp termino bien
			                currentSize += strlen(success);//Aumentamos el tamano del string log segun el largo del string success
				            log = realloc(log, currentSize * sizeof(char));
				            strcat(log,success);

			            } else { //El excecvp termino mal
			                currentSize += strlen(failed);//Aumentamos el tamano del string log segun el largo del string failed
				            log = realloc(log, currentSize * sizeof(char));
				            strcat(log,failed);
			            }
			        }
			    } else { // Agrega proceso a la lista de procesos background
			    	struct process *hijo = (struct process*)malloc(sizeof(struct process));
		        	hijo->next = childrenBackground->next;
		        	childrenBackground->next = hijo;
		        	hijo->_pid = pid;
		        	hijo->prioridad = DEFAULT;
		        	printf("[ ... ] %i\n",pid);
			    }
	        }
	    }
        background = FALSE;
    } 
    fclose(fptr); //Cerramos el archivo y se mostraran los cambios realizados en el
    return 0;
}