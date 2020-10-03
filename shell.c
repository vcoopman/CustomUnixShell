#include <stdlib.h>   
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 100
#define MAX_N_PARAMS 10

void parse(char* input, char** params) { //Dividimos el input en un arreglo de parametros
    for(int i=0; i<MAX_N_PARAMS; i++) {
        params[i] = strsep(&input, " ");//Se guardan los comandos separados por espacio en el arreglo de paramteros
        if(params[i] == NULL) break;
    }
    return;
};

int execute(char** params, char* log) {
    pid_t pid = fork(); //Fork

    if (pid == -1) { //Error
        printf("Error haciendo fork!!\n");
        return 1;
    } else if (pid == 0) { // Proceso hijo
        execvp(params[0], params); //Ejecuta comando
        printf("Comando Desconocido\n");
        _exit(127); //Este exit es necesario para saber si fallo el execvp
        return 1;
    } else { // Proceso padre
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
    }
};

int main() {    
    char input[MAX_INPUT_LENGTH+1];    
    char * params[MAX_N_PARAMS+1];

    int currentSize = 1;
    char *success = " Success\n\0"; //Mensaje de comando satisfactorio en el log
    char *failed = " Failed\n\0"; //Mensaje de comando erroneo en el log
    char *log = malloc(sizeof(char)); //Creamos el log, el cual es un string dinamico
    FILE *fptr; //Puntero a tipo archivo
    fptr = fopen("mishell.log","a"); //Abrimos el archivo en modo append (para escribir sobre lo que existe), de no existir es creado
    if (fptr == NULL)
    { 
        printf("Error abriendo el archivo log"); 
        return 0; 
    } 

    while(1) {
        printf("CustomUnixShell $:"); //Prompt   
        if(fgets(input, sizeof(input), stdin) == NULL) break; //Input vacio con CTRL+D cierra la shell
        if(input[0] == '\n') continue; //Si se presiona enter sin un comando previo, imprime denuevo la shell
        if(input[strlen(input)-1] == '\n') { // Se remueve el caracter de salto de linea al final del input 
            input[strlen(input)-1] = '\0';    
        }
        currentSize += strlen(input); //Aumentamos el tamano del string log segun el largo del input
        log = realloc(log, currentSize * sizeof(char));
        strcat(log, input); // Concatenamos el input al log
        parse(input, params); //Dividimos el input en un arreglo de parametros  
        if (!strcmp(params[0], "log")){ //Se crea el archivo de log con los datos recopilados 
            fprintf(fptr,"%s\n",log); //Guardamos el log en el archivo
            continue;
        }     
        if (strcmp(params[0], "exit") == 0) break; // Comando exit  
        if (execute(params, log) == 0) { //Comando correcto
            currentSize += strlen(success);//Aumentamos el tamano del string log segun el largo del string success
            log = realloc(log, currentSize * sizeof(char));
            strcat(log,success);
        } else { //Comando incorrectos
            currentSize += strlen(failed);//Aumentamos el tamano del string log segun el largo del string success
            log = realloc(log, currentSize * sizeof(char));
            strcat(log,failed);
        }
    } 
    fclose(fptr); //Cerramos el archivo y se mostraran los cambios realizados en el
    return 0;
}