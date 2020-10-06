#include <stdlib.h>   
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 100
#define MAX_N_PARAMS 10
#define MAX_N_COMMANDS 3
#define TRUE 1
#define FALSE 0
#define DEFAULT 1

static int background = FALSE; // la funcion parse puede cambiar este valor. En cada ciclo vuelve a ser FALSE

struct process{
	pid_t _pid;
	int prioridad;
	struct process *next;
};

struct command {
	char **argv;
};

struct command * parse(char* input) { //Dividimos el input en un arreglo de comandos con parametros
	static struct command results[MAX_N_COMMANDS]; // (Global)
	char *auxParam = NULL;

	/* Crear arreglos de strings */
	for(int i = 0; i < MAX_N_COMMANDS ; ++i){
		results[i].argv = (char**)malloc(MAX_N_PARAMS * sizeof(char*));
		for(int j = 0; j < MAX_N_PARAMS; ++j){
			results[i].argv[j] = (char*)malloc(sizeof(char));
			results[i].argv[j] = NULL;
		}	
	}

	/* Parsing */
    for(int i = 0; i < MAX_N_COMMANDS; i++) {
    	for(int j = 0; j < MAX_N_PARAMS; ++j){

	        auxParam = strsep(&input, " "); // Se va analizando el input, separando comandos y parametros
	        if (auxParam == NULL){
	        	results[i].argv[j] = auxParam; // Se guarda este null para tener claro donde termina el comando
	        	return results;
	        }
	        else if (strcmp(auxParam,"|") == 0 ) break; // Pasa a guardar en el siguiente comando, no guarda el "|"
	        else if (strcmp(auxParam,"&") == 0 ) background = TRUE;
	        
	        results[i].argv[j] = auxParam;
		}
    }
};

void printList(struct process *node){
	while(node != NULL){
		printf("[%i] \n",node->_pid);
		node = node->next;
	}
}

void printCommands(struct command *commands){
	for(int i = 0; i < MAX_N_COMMANDS; ++i){
		int j = 0;
		while(commands[i].argv[j] != NULL){
			printf("%s ",commands[i].argv[j]);
			++j;
		}
		printf("\n");
	}
}

int main() {    
    char input[MAX_INPUT_LENGTH+1];    
   	struct command *commands;

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

        /* Preparar el contenido a loguear */
        currentSize += strlen(input); //Aumentamos el tamano del string log segun el largo del input
        log = realloc(log, currentSize * sizeof(char));
        strcat(log, input); // Concatenamos el input al log

  		/* Parse input */
        commands = parse(input); //Dividimos el input en un arreglo de parametros
        printCommands(commands);

        /* Realizar log de comandos */
        if (!strcmp(commands[0].argv[0], "log")){ //Se crea el archivo de log con los datos recopilados 
            fprintf(fptr,"%s\n",log); //Guardamos el log en el archivo
            continue;
        }     

        /* Caso exit */
        if (strcmp(commands[0].argv[0], "exit") == 0){
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

        /* Ejecutar comandos */
        if(commands[0].argv[0] != NULL){
        	pid_t pid = fork();

        	if (pid < 0){ // Error
	        		printf("No se pudo iniciar proceso\n");
	        		exit(1); 

	        } else if (pid == 0){
	        		/* SOLO ESTA EJECUTANDO EL PRIMER COMANDO */
	        		execvp(commands[0].argv[0], commands[0].argv);

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