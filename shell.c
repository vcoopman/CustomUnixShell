#include <stdlib.h>   
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

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

struct nodoPq{
    char command[20];
    struct nodoPq *siguiente;
    int prioridad;
};

bool searchPq(struct nodoPq* head, char* command){
    struct nodoPq* it=head;
    while(it->siguiente!=NULL){
        if(strcmp(it->command,command)==0){
            return true;
        }
        it=it->siguiente;
    }
    if(strcmp(it->command,command)==0){
        return true;
    }
    return false;
}
void deletePq(struct nodoPq* head){
    struct nodoPq* sig;
    struct nodoPq* it=head;
    if(it->siguiente==NULL){
        free(it);
        return;
    }
    while(it->siguiente->siguiente!=NULL){
        sig=it->siguiente;
        free(it);
        it=sig;
        sig=it->siguiente;

    }
    sig=it->siguiente;
    free(it);
    free(sig);
}
char* getCommand(struct nodoPq* head, int pos){
    int count=1;
    struct nodoPq* it=head;
    while(it->siguiente!=NULL){
        if(count==pos){
            return it->command;
        }else{
            it=it->siguiente;
            count++;
        }
    }
    if(count==pos){
        return it->command;
    }
}
//Ordena la cola de prioridad
void sortPq(struct nodoPq* head){
    struct nodoPq* it;
    struct nodoPq* pivote=head;
    struct nodoPq* max=NULL;
    struct nodoPq aux;
    while(pivote->siguiente!=NULL){
        it=pivote;
        max=NULL;
        while(it->siguiente!=NULL){
            if(it->prioridad>pivote->prioridad){
                max=it;
            }
            it=it->siguiente;
        }
        
        if(it->prioridad>pivote->prioridad){
            max=it;
        }
        if(max!=NULL){
            //swap
            aux=*max;
            *max=*pivote;
            *pivote=aux;
            pivote->siguiente=max->siguiente;
            max->siguiente=aux.siguiente;
        }
        pivote=pivote->siguiente;
    }
}
//Inserta en la cola de prioridad
int insertPq(struct nodoPq* head, char* command){
    struct nodoPq* it=head;
    struct nodoPq* insert;
    while(it->siguiente!=NULL){
        if(strcmp(it->command,command)==0){
            it->prioridad++;
            sortPq(head);
            return 1;
        }
        it=it->siguiente;
    }
    if(strcmp(it->command,command)==0){
        it->prioridad++;
        sortPq(head);
        return 1;
    }
    insert=(struct nodoPq*)malloc(sizeof(struct nodoPq));
    insert->siguiente=NULL;
    it->siguiente=insert;
    strcpy(insert->command,command);
    insert->prioridad=1;
    return 0;
}
//Muestra la colad de prioridad
void displayPq(struct nodoPq* head){
    struct nodoPq* it=head;
    int count=0;
    while(it->siguiente!=NULL){
        count++;
        printf("\033[0;32m%d) %s\n",count,it->command);
        it=it->siguiente;
    }
    count++;
    printf("\033[0;32m%d) %s\n",count,it->command);
    printf("\033[0m");
}
//Modifica la prioridad de un comando
void modPriority(struct nodoPq* head, char* command, int prioridad){
    //Buscar nodo
    struct nodoPq* it=head;
    while(it->siguiente!=NULL){
        if(strcmp(it->command,command)==0){
            it->prioridad=prioridad;
            sortPq(head);
            return;
        }else{
            it=it->siguiente;
        }
    }
    if(strcmp(it->command,command)==0){
        it->prioridad=prioridad;
        sortPq(head);
        return;
    }
    printf("Comando no existe en la lista");
}

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
/*
void printCommands(struct command *commands){
	for(int i = 0; i < MAX_N_COMMANDS; ++i){
		int j = 0;
		while(commands[i].argv[j] != NULL){
			printf("%s ",commands[i].argv[j]);
			++j;
		}
		printf("\n");
	}
}*/

int main() {    
    char input[MAX_INPUT_LENGTH+1];    
   	struct command *commands;
	struct nodoPq* pq;
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
    int primNode=1;
    int maxNodes=20;
    int countNodes=0;
  	/* Open log file */
    FILE *fptr; //Puntero a tipo archivo
    fptr = fopen("mishell.log","a"); //Abrimos el archivo en modo append (para escribir sobre lo que existe), de no existir es creado
    pid_t pid = 0;
    int pipefd[2];
    char outp[2000];
	
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
	printf("\033[0;36mCustomUnixShell$:");
        printf("\033[0m");
        /* Input vacio con CTRL+D cierra la shell (???)*/
        if(fgets(input, sizeof(input), stdin) == NULL) break; 

        /* Caso enter */
        if(input[0] == '\n') continue; //Si se presiona enter sin un comando previo, imprime de nuevo la shell

        /* Se remueve el caracter de salto de linea al final del input */
        if(input[strlen(input)-1] == '\n') { 
            input[strlen(input)-1] = '\0';
	    //Esto deberia ir en otra parte para que si se ingresa cualquier cosa que no sea comando
		//No lo inserte en la lista
	    if(primNode && countNodes<maxNodes){
                pq=(struct nodoPq*)malloc(sizeof(struct nodoPq));
                strcpy(pq->command,input);
                pq->prioridad=1;
                pq->siguiente=NULL;
                primNode=0;
                countNodes++;
            }else{
                if(countNodes>=maxNodes){
                    //printf("No se puede insertar mas\n");
                }else if(insertPq(pq,input)){
                    //printf("Ya se encuentra en la lista\n");
                }else{
                    ///printf("Insertado correctamente\n");
                    countNodes++;
                }
            }
        }

        /* Preparar el contenido a loguear */
        currentSize += strlen(input); //Aumentamos el tamano del string log segun el largo del input
        log = realloc(log, currentSize * sizeof(char));
        strcat(log, input); // Concatenamos el input al log

 	/* Parse input */
        commands = parse(input); //Dividimos el input en un arreglo de parametros
	/* Imprime comandos (testing) */
	// printCommands(commands);

        /* Realizar log de comandos */
	if(!strcmp(commands[0].argv[0], "execAgain")){
            int sel=0;
            printf("Que comando desea ejecutar?\n");
            displayPq(pq);
            scanf("%d",&sel);
            while(sel<=0 || sel>countNodes){
                displayPq(pq);
                printf("Ingrese una opción dentro de la lista:");
                scanf("%d",&sel);
            }
            strcpy(input,getCommand(pq,sel));
            commands=parse(input);
        }
        if (!strcmp(commands[0].argv[0], "log")){ //Se crea el archivo de log con los datos recopilados 
            fprintf(fptr,"%s\n",log); //Guardamos el log en el archivo
            continue;
        }else if(!strcmp(commands[0].argv[0], "cmdList")){//Imprime la lista
            displayPq(pq);
            continue;
        }else if(!strcmp(commands[0].argv[0], "modPr")){//modifica una prioridad
            int sel=0;
            int pr;
            printf("Que comando desea modificar?\n");
            displayPq(pq);
            scanf("%d",&sel);
            while(sel<=0 || sel>countNodes){
                displayPq(pq);
                printf("Ingrese una opción dentro de la lista:");
                scanf("%d",&sel);
            }
            printf("Inserte el valor nuevo de prioridad:");
            scanf("%d",&pr);
            strcpy(input,getCommand(pq,sel));
            modPriority(pq,input,pr);
            printf("\033[0;35mPrioridad has been changed successfully\n");
            printf("\033[0m");
            continue;
        }else if(!strcmp(commands[0].argv[0], "maxCmd")){
            //Ingresa la cantidad maxima de comandos en la lista
            printf("Inserte el valor maximo de comandos en la lista:");
            scanf("%d",&maxNodes);
            while(maxNodes<=0){
                printf("Inserte un valor mayor que 0:");
                scanf("%d",&maxNodes);
            }
            //Si hay mas comandos una vez que se modifica a un maximo menor
            //se elimina completa (deberia eliminar los sobrantes)
            if(maxNodes<countNodes){
                deletePq(pq);
                primNode=1;
                countNodes=0;
            }
            continue;     
        /* Caso exit */
	}else if (strcmp(commands[0].argv[0], "exit") == 0){
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
				//Se capturan los extremos del pipe para lectura en el padre
                    		dup2(pipefd[1], STDOUT_FILENO);
                    		close(pipefd[0]);
                    		close(pipefd[1]);
	        		execvp(commands[0].argv[0], commands[0].argv);

	        		/* En el caso que falle el execvp */
			        printf("Comando Desconocido\n");
			        _exit(127); //Este exit es necesario para saber si fallo el execvp, Exit de command not found

	        } else {
	        	if (!background){ // Espera por el proceso
	        		int status; 
	        		int wait = waitpid(pid, &status, 0);
	        		FILE *f;
                    		close(pipefd[1]);
                    		int nbytes= read(pipefd[0],outp,sizeof(outp));//Se alamacena el output del comando
                    		printf("\033[0;32m%.*s",nbytes,outp);//Se imprime de colores
                    		printf("\033[0m");
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
    deletePq(pq);
    return 0;
}
