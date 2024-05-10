#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int children(int n, int buffer, int start, int curr, int pipes[n][2]);

int main(int argc, char **argv)
{	
	int start, n;
	int buffer[1];
	
	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n\n"); exit(0);}
    
    /* Parsing of arguments */
	n = atoi(argv[1]);
	buffer[0] = atoi(argv[2]);
	start = atoi(argv[3]);
	
	int pipes[n][2];
	for (int i = 0; i < n; i++) {
		if (pipe(pipes[i]) == -1) {
			printf("Error al generar el pipe %i\n\n", i);
			return -1;
		}
	}
	printf("%i pipes fueron creados exitosamente\n\n", n);
    printf("Se crearán %i procesos, se enviará el número %i al proceso %i \n\n", n, buffer[0], start);

	int pids[n];
	for (int i = 0; i < n; i++) {
		pids[i] = fork();
		if (pids[i] == 0) {
			return children(n, buffer[0], start, (i + start)%n, pipes);
		}
	}
	for (int i = 0; i < n; i++) {
		waitpid(pids[i], NULL, 0);
	}
	int result = 0;
	read(pipes[start][0], &result, sizeof(int));
	printf("El número que llegó al proceso %i fue el %i", start, result);
	return result;
}

int children(int n, int buffer, int start, int curr, int pipes[n][2]) {
	if (curr == start) {
		int num = buffer + 1;
		write(pipes[(curr + 1) % n][1], &num, sizeof(int));
		printf("El proceso %i le pasó el número %i al proceso %i\n\n", curr, buffer + 1, curr+1); //
		return buffer;
	}
	int num;
	read(pipes[curr][0], &num, sizeof(int));
	printf("El proceso %i recibió el número %i\n", curr, num);
	num++;
	write(pipes[(curr+1)%n][1], &num, sizeof(int));
	printf("El proceso %i le pasó el número %i al proceso %i\n\n", curr, num, curr+1);
	return num;
}