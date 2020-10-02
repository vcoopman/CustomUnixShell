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

int execute(char** params) {
    pid_t pid = fork(); //Fork

    if (pid == -1) { //Error
        printf("Error haciendo fork!!\n");
        return 1;
    } else if (pid == 0) { // Proceso hijo
        execvp(params[0], params); //Ejecuta comando
        printf("Comando Desconocido\n");
        return 0;
    } else { // Proceso padre
        int child;
        waitpid(pid, &child, 0); //Espera a que termine el hijo
        return 1;
    }
};

int main() {    
    char input[MAX_INPUT_LENGTH+1];    
    char * params[MAX_N_PARAMS+1];     
    while(1) {
        printf("CustomUnixShell $:"); //Prompt   
        if(fgets(input, sizeof(input), stdin) == NULL) break; //Input vacio con CTRL+D cierra la shell
        if(input[0] == '\n') continue; //Si se presiona enter sin un comando previo, imprime denuevo la shell
        if(input[strlen(input)-1] == '\n') { // Se remueve el caracter de salto de linea al final del input 
            input[strlen(input)-1] = '\0';    
        }    
        parse(input, params); //Dividimos el input en un arreglo de parametros        
        if (strcmp(params[0], "exit") == 0) break; // Comando exit  
        if (execute(params) == 0) break;
    } // end while
    return 0;
}