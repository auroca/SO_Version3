#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql/mysql.h>
#include <pthread.h>

typedef struct{
	char nombre[20];
	int socket;
}conectado;

typedef struct{
	conectado conectados[100];
	int num;
}listaconectados;

listaconectados jugadoresconectados;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// ==================== FUNCIONES MYSQL ====================

// Función para conectarse a la base de datos
MYSQL* conectar_bd()
{
	MYSQL* conn = mysql_init(NULL);
	if (conn == NULL)
	{
		printf("Error al inicializar MySQL\n");
		exit(1);
	}
	if (!mysql_real_connect(conn, "localhost", "root", "mysql", "game", 0, NULL, 0))
	{
		printf("Error al conectar con MySQL: %s\n", mysql_error(conn));
		exit(1);
	}
	return conn;
}

int pon (char nombre[20], int socket){
	//añidir nuevo conectado si la lista no es llena
	if (jugadoresconectados.num == 100)
		return -1;
	else{
		strcpy(jugadoresconectados.conectados[jugadoresconectados.num].nombre, nombre);
		jugadoresconectados.conectados[jugadoresconectados.num].socket=socket;
		jugadoresconectados.num++;
		return 0;
	}
}

int pos (char nombre[20]){
	int encontrado=0;
	int i=0;
	while((i<jugadoresconectados.num-1) || (encontrado==0)){
		if (strcmp(jugadoresconectados.conectados[i].nombre, nombre))
			encontrado=1;
		else
			i++;
	}
	if (encontrado==0)
		return -1;
	else
		return i;
}

int elimina (char nombre[20]){
	int p = pos(nombre);
	if (p==-1)
		return -1;
	else{
		for(int i=p-1; i<jugadoresconectados.num-1; i++){
			jugadoresconectados.conectados[i] = jugadoresconectados.conectados[i+1];
		}
		jugadoresconectados.num--;
		return 0;
	}
}

void DameConectados (char resultado[512]){
	sprintf(resultado, "%d", jugadoresconectados.num);
	for(int i=0; i<jugadoresconectados.num; i++){
		sprintf(resultado, "%s/%s", resultado, jugadoresconectados.conectados[i].nombre);
	}
}

// Registro de usuario
void registrar_usuario(char* usuario, char* password, char* respuesta)
{
	MYSQL* conn = conectar_bd();
	char query[512];
	sprintf(query, "INSERT INTO Player (username, password) VALUES ('%s', '%s')", usuario, password);
	
	printf("ciao");
	
	if (mysql_query(conn, query) == 0)
	{
		printf("Registro exitoso.\n");
		strcpy(respuesta, "Registro exitoso.");
	}
	else
	{
		printf("Error en el registro: %s\n", mysql_error(conn));
		strcpy(respuesta, "Error en el registro.");
	}
	mysql_close(conn);
}

// Login de usuario
void login_usuario(char* usuario, char* password, char* respuesta)
{
	MYSQL* conn = conectar_bd();
	char query[512];
	sprintf(query, "SELECT username, password FROM Player WHERE username ='%s' AND password= '%s'", usuario, password);
	
	if (mysql_query(conn, query) == 0)
	{
		MYSQL_RES* res = mysql_store_result(conn);
		if (mysql_fetch_row(res) == NULL)
			strcpy(respuesta, "Usuario o contraseña incorrectos.");
		else
			strcpy(respuesta, "Login exitoso.");
		mysql_free_result(res);
	}
	else
	{
		strcpy(respuesta, "Error en la consulta.");
	}
	mysql_close(conn);
}

// Número de partidas jugadas por un usuario
void num_partidas_jugadas(char* usuario, char* respuesta)
{
	MYSQL* conn = conectar_bd();
	char query[512];
	sprintf(query, "SELECT Game FROM Participation WHERE Player = '%s'", usuario);
	
	if (mysql_query(conn, query) == 0)
	{
		MYSQL_RES* res = mysql_store_result(conn);
		MYSQL_ROW row;
		int num_partidas = 0;
		while ((row = mysql_fetch_row(res)) != NULL)
		{
			num_partidas++;
		}
		sprintf(respuesta, "Partidas jugadas: %d", num_partidas);
		mysql_free_result(res);
	}
	else
		strcpy(respuesta, "Error en la consulta.");
	
	mysql_close(conn);
}

// Puntuación máxima de un jugador
void puntuacion_maxima(char* usuario, char* respuesta)
{
	MYSQL* conn = conectar_bd();
	char query[512];
	sprintf(query, "SELECT MAX(points) FROM Game WHERE GameID IN (SELECT Game FROM Participation WHERE Player='%s')", usuario);
	
	if (mysql_query(conn, query) == 0)
	{
		MYSQL_RES* res = mysql_store_result(conn);
		MYSQL_ROW row = mysql_fetch_row(res);
		sprintf(respuesta, "Puntuacion maxima: %s", row[0] ? row[0] : "0");
		mysql_free_result(res);
	}
	else
		strcpy(respuesta, "Error en la consulta.");
	
	mysql_close(conn);
}

// Lista de jugadores con los que ha jugado un usuario
void jugadores_con_partidas(char* usuario, char* respuesta)
{
	MYSQL* conn = conectar_bd();
	char query[512];
	sprintf(query, "SELECT DISTINCT Player FROM Participation WHERE Game IN (SELECT Game FROM Participation WHERE Player='%s') AND Player != '%s'", usuario, usuario);
	
	if (mysql_query(conn, query) == 0)
	{
		MYSQL_RES* res = mysql_store_result(conn);
		MYSQL_ROW row;
		strcpy(respuesta, "Jugadores: ");
		while ((row = mysql_fetch_row(res)) != NULL)
		{
			strcat(respuesta, row[0]);
			strcat(respuesta, ", ");
		}
		mysql_free_result(res);
	}
	else
		strcpy(respuesta, "Error en la consulta.");
	
	mysql_close(conn);
}

// ==================== SERVIDOR PRINCIPAL ====================

void *AtenderCliente (void *socket)
{
	int sock_conn = *(int*)socket;
	int sock_listen;
	
	char peticion[512];
	char respuesta[512];
	
	int jugconectado=0;
	char usuario[50], password[50];
	
	int terminar=0;	
	
	while (terminar==0)
	{
		int ret = read(sock_conn, peticion, sizeof(peticion));
		if (ret <= 0)
		{
			// El cliente se ha desconectado o ocurrió un error
			printf("El cliente se ha desconectado o se produjo un error en read\n");
			break;
		}
		peticion[ret] = '\0';
		printf("Petición recibida: %s\n", peticion);
		
		// Analizar la petición
		char* p = strtok(peticion, "/");
		if (p == NULL)
		{
			strcpy(respuesta, "Formato de petición incorrecto.");
			write(sock_conn, respuesta, strlen(respuesta));
			continue;
		}
		int codigo = atoi(p);
		
		p = strtok(NULL, "/");
		if ((p == NULL) && (codigo!=4))
		{
			strcpy(respuesta, "Formato de petición incorrecto.");
			write(sock_conn, respuesta, strlen(respuesta));
			continue;
		}
		else if (codigo!=4)
			strcpy(usuario, p);
		
		// Procesar según el código
		if (codigo == 0)  // Registro de usuario
		{
			p = strtok(NULL, "/");
			if (p == NULL)
				strcpy(respuesta, "Formato de petición incorrecto.");
			else
			{
				strcpy(password, p);
				registrar_usuario(usuario, password, respuesta);
				pthread_mutex_lock (&mutex);
				int r = pon(usuario, socket);
				if (r==-1)
					printf("Lista esta llena \n");
				else
					printf("Añidido correctamente \n");
				pthread_mutex_unlock (&mutex);
				jugconectado=1;
			}
		}
		else if (codigo == 9) // Login de usuario
		{
			p = strtok(NULL, "/");
			if (p == NULL)
				strcpy(respuesta, "Formato de petición incorrecto.");
			else
			{
				strcpy(password, p);
				
				printf("Ejecutando login_usuario con: %s / %s\n", usuario, password);
				fflush(stdout); // Asegura que se imprime de inmediato
				
				login_usuario(usuario, password, respuesta);
				pthread_mutex_lock (&mutex);
				int r = pon(usuario, socket);
				if (r==-1)
					printf("Lista esta llena \n");
				else
					printf("Añidido correctamente \n");
				pthread_mutex_unlock (&mutex);
				jugconectado=1;
			}
		}
		else if (codigo == 1)  // Número de partidas jugadas
		{
			num_partidas_jugadas(usuario, respuesta);
		}
		else if (codigo == 2)  // Puntuación máxima de un jugador
		{
			puntuacion_maxima(usuario, respuesta);
		}
		else if (codigo == 3)  // Jugadores con los que ha jugado
		{
			jugadores_con_partidas(usuario, respuesta);
		}
		else if (codigo == 4){
			pthread_mutex_lock (&mutex);
			DameConectados(respuesta);
			pthread_mutex_unlock (&mutex);
		}
		else
		{
			strcpy(respuesta, "Comando no valido.");
		}
		
		// Enviar respuesta al cliente
		write(sock_conn, respuesta, strlen(respuesta));
		fflush(stdout);
	}
	close(sock_conn);
	if (jugconectado==1){
		pthread_mutex_lock (&mutex);
		int r = elimina(usuario);
		if (r == -1)
			printf("el jugador no esta en la lista \n");
		else
			printf("jugador eliminato \n");
		pthread_mutex_unlock (&mutex);
	}
	return 0;
	
}

int main()
{
	int sock_conn, sock_listen;
	struct sockaddr_in serv_adr;
	
	
	// Crear socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Error creando socket\n");
		return 1;
	}
	
	// Configuración del socket
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(9050);
	
	// Bind al puerto
	if (bind(sock_listen, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) < 0)
	{
		printf("Error en el bind\n");
		return 1;
	}
	
	// Escuchar conexiones
	if (listen(sock_listen, 3) < 0)
	{
		printf("Error en el listen\n");
		return 1;
	}
	
	int i;
	int sockets[100];
	jugadoresconectados.num=0;
	pthread_t thread;
	i=0;
	
	for (;;)
	{
		printf("Servidor en espera de conexiones...\n");
		sock_conn = accept(sock_listen, NULL, NULL);		
		if (sock_conn < 0)
		{
			printf("Error en accept\n");
			continue;
		}
		printf("Nueva conexión establecida\n");
		
		sockets[i] = sock_conn;		
		pthread_create (&thread, NULL, AtenderCliente,&sockets[i] );
		i++;
	}
	
	//for (i=0; i<5;i++)
	//{
		//pthread_join (thread[i], NULL);
	//}
	
}
