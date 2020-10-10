# CustomUnixShell
Estudiar manejo de procesos concurrentes en Unix, creación, ejecución y terminación usando llamadas a sistemas fork(), exec() y wait().  Además el uso de otras llamadas a sistema como signals y comunicación entre procesos usando pipes.

TO-DO

(a) (X)  La shell debe proporcionar un prompt, lo que identifica el modo de espera de comandos de la shell.

(b) (X)  Debe leer un comando desde teclado y parsear la entrada para identificar el comando y sus argumentos (debe soportar al menos 3 argumentos). //Por ejemplo: wc -c -m shell.c

(c) (X)  Debe ejecutar el comando ingresado en un proceso concurrente, para lo cual debe usar el llamado a sistema fork() y algunas de las variantes de exec(). Los comandos a soportar son ejecutados en foreground, es decir, la shell ejecuta y espera por el termino de su ejecucion antes de imprimir el promtp para esperar por el siguiente comando. //Por ejemplo: sleep 3

(d) (X)  Si  se  presiona  “enter“  sin  previo  comando,  la  shell  simplemente  imprime  nuevamente  el prompt.

(e) (X)  Debe proporcionar la salida de la ejecucion del comando en distinto color.

(f) (X)  Su shell debe crear un comando que le permita crear un archivo mishell.log con el registro de los comandos ejecutados por sesion. Este comando puede ir agregando la informacion de distintas sesiones al final del existente o puede ser reescrito.  //El log es guardado con el comando: log    , los cambios son visualizados tras salir de la terminal

(g) (X)  Su shell ademas debe soportar el comando exit para salir de la shell. 

(h) (X)  Debe poder continuar si es que un comando ingresado no existe, proporcionando el error correspondiente. //No especifica el error, solo muestra "Comando Desconocido"

(i) (X)  Debe manejar procesos en background (comandos que se terminan con caracter &). Esto esque la shell no espere por el termino del comando, de manera que la shell pueda continuar ejecutando otros comandos. En este caso,  su shell debe registrar estos procesos de tal manera que si el usuario quiera terminar la shell, ́esta le proporcione la lista de procesos activos en background. Notar que aca no se pide hacer que un proceso background pase a foreground. //Ejemplo: ls &, y luego exit

(j) (X)  Manejar una cola de prioridades con los comandos mas frequentemente utilizados.  Asuma que tiene un maximo de k, cuyo valor se configura con un comando especial de su shell1.

(k) (X)  Desplegar por pantalla los comandos en su cola de prioridades.

(l) (X)  Elegir un comando de la lista y volver a ejecutarlo.

(m) (X)  Finalmente, investigue lo que son los procesos defuncty agregue a su shell un comando que cree N procesos defunct y un comando que permita eliminarlos.

(n) (X) Su shell debe considerar comandos que contengan pipes, es decir, del tipops -la|grep PRI, para ello debe utilizar las llamadas a sistema pipe(), dup() o dup2(), and close().Su shell debe soportar m ́ultiples pipes en un comando dado, por ejemplo:ls -l|grep file|wc -l
