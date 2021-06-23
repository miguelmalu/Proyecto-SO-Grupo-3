#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mysql.h>
#include <pthread.h>
#include <my_global.h> //Solo para BD en Entorno Producci�n

//ESTRUCTURAS PARA LA LISTA DE CONECTADOS
typedef struct {
	char nombre [20];
	int socket;
} Conectado;

typedef struct {
	Conectado conectados [100];
	int num;
} ListaConectados;

ListaConectados lista;

//FUNCIONES PARA LA LISTA DE CONECTADOS
int Pon (ListaConectados *lista, char nombre[20], int socket) {
	//A�ade nuevos conectados.
	//Retorna 0 si OK, 1 si la lista estaba llena.
	if (lista->num == 100)
		return -1;
	else {
		strcpy (lista->conectados[lista->num].nombre, nombre);
		lista->conectados[lista->num].socket = socket;
		lista->num++;
		return 0;
	}
}

int DamePosicion (ListaConectados *lista, char nombre[20]) {
	//Devuelve la posici�n o -1 si no est� en la lista
	int i=0;
	int encontrado=0;
	while ((i < lista->num) && !encontrado) {
		if (strcmp (lista->conectados[i].nombre, nombre) ==0)
			encontrado =1;
		if (!encontrado)
			i++;
	}
	if (encontrado)
		return i;
	else
		return -1;
}

int DamePosicionPorSocket (ListaConectados *lista, int socket) {
	//Devuelve la posici�n o -1 si no est� en la lista
	int i=0;
	int encontrado=0;
	while ((i < lista->num) && !encontrado) {
		if (lista->conectados[i].socket == socket)
			encontrado =1;
		if (!encontrado)
			i++;
	}
	if (encontrado)
		return i;
	else
		return -1;
}

int Elimina (ListaConectados *lista, char nombre[20]) {
	//Retorna 0 si elimina, -1 si no est� en la lista
	int pos = DamePosicion (lista, nombre);
	if (pos == -1)
		return -1;
	else {
		for (int i=pos; i < lista->num-1; i++)
			lista->conectados[i] = lista->conectados[i+1];
		lista->num--;
		return 0;
	}
}

int EliminaPorSocket (ListaConectados *lista, int socket) {
	//Retorna 0 si elimina, -1 si no est� en la lista
	int pos = DamePosicionPorSocket (lista, socket);
	if (pos == -1)
		return -1;
	else {
		for (int i=pos; i < lista->num-1; i++)
			lista->conectados[i] = lista->conectados[i+1];
		lista->num--;
		return 0;
	}
}

void DameConectados (ListaConectados *lista, char conectados[300]) {
	//Pone en conectados el nombre de todos los conectados separados por /
	//Empieza con el n�mero de conectados, Ejemplo: "3/Juan/MAria/Pedro"
	sprintf (conectados, "%d", lista->num);
	for (int i=0; i < lista->num; i++)
		sprintf (conectados, "%s/%s", conectados, lista->conectados[i].nombre);
}

//ESTRUCTURAS PARA LA TABLA DE PARTIDAS
typedef struct {
	char u_j1 [20]; //Jugador1: Anfitri�n
	int s_j1;
	char u_j2 [20]; //Jugador2: Invitado1
	int s_j2;
	char u_j3 [20];
	int s_j3;
	char u_j4 [20];
	int s_j4;
} Partida;

typedef Partida TablaPartidas [20];

TablaPartidas tabla;

//VARIABLES PARA LA CREACI�N DE PARTIDAS
int invitados;
int respuestasinvitaciones;
int invitadoseliminados;

//FUNCIONES PARA LA TABLA DE PARTIDAS
void InicializarTabla (TablaPartidas tabla) {
	//Pone la marca de libre al socket del anfitri�n de toda la tabla
	//Siendo la tabla un vector, C la pasa autom�ticament por referencia
	for (int i=0; i<20; i++) {
		tabla[i].s_j1 = -1;
		tabla[i].s_j2 = -1;
		tabla[i].s_j3 = -1;
		tabla[i].s_j4 = -1;
	}
}

int CrearPartida (ListaConectados *lista, char invitados[300], TablaPartidas tabla, int socket) {
	//Crea un Partida con los par�metros que recibe. Retorna la posici�n en la tabla, o error si no es posible crearla.
	int encontrado = 0;
	int i=0;
	int pos=0;
	while ((i<20) && !encontrado)
	{
		if (tabla[i].s_j1 == -1)
			encontrado = 1;
		if (!encontrado)
			i++;
	}
	if (encontrado) {
		pos = DamePosicionPorSocket(lista, socket);
		strcpy(tabla[i].u_j1, lista->conectados[pos].nombre);
		tabla[i].s_j1 = lista->conectados[pos].socket;
		char *p = strtok( invitados,",");
		for (int j=2; (p != NULL) && (j < 5) && (pos != -1); (j++) && (p = strtok( NULL,","))) {
			pos = DamePosicion(lista, p);
			if (pos !=-1) {
				if (j == 2) {
					strcpy(tabla[i].u_j2, lista->conectados[pos].nombre);
					tabla[i].s_j2 = lista->conectados[pos].socket;
				} else if (j == 3) {
					strcpy(tabla[i].u_j3, lista->conectados[pos].nombre);
					tabla[i].s_j3 = lista->conectados[pos].socket;
				} else if (j == 4) {
					strcpy(tabla[i].u_j4, lista->conectados[pos].nombre);
					tabla[i].s_j4 = lista->conectados[pos].socket;
				}
			}
		}
		printf ("CrearPartida: %s/%d/%s/%d/%s/%d/%s/%d\n", tabla[i].u_j1,tabla[i].s_j1,
				tabla[i].u_j2,tabla[i].s_j2,tabla[i].u_j3,tabla[i].s_j3,
				tabla[i].u_j4, tabla[i].s_j4); //Debug
		if (pos != -1) {
			if ((tabla[i].u_j2 != "") || (tabla[i].s_j2 != -1))
				return i;
			else
				return -2;
		} else
			return -1;
	} else
		return -3;
}

int BuscarPosicionPartidaPorJugador (TablaPartidas tabla, char jugador [20]) {
	//Si existe, devuelve la posici�n en la tabla del jugador cuyo nombre recibe como par�metro, si no, -1.
	int encontrado = 0;
	int i=0;
	while ((i<20) && ! encontrado) {
		for (int j=1; j < 5; j++) {
			if (j == 1) {
				if (strcmp (tabla[i].u_j1,jugador) == 0)
					encontrado = 1;
			} else if (j == 2) {
				if (strcmp (tabla[i].u_j2,jugador) == 0)
					encontrado = 1;
			} else if (j == 3) {
				if (strcmp (tabla[i].u_j3,jugador) == 0)
					encontrado = 1;
			} else if (j == 4) {
				if (strcmp (tabla[i].u_j4,jugador) == 0)
					encontrado = 1;
			}
		}
		if (!encontrado)
			i++;
	}
	if (encontrado)
		return i;
	else
		return -1;
}

int BuscarPosicionPartidaPorSocket (TablaPartidas tabla, int socket) {
	//Si existe, devuelve la posici�n en la tabla del jugador cuyo socket recibe como par�metro, si no, -1.
	int encontrado = 0;
	int i=0;
	while ((i<20) && ! encontrado) {
		for (int j=1; j < 5; j++) {
			if (j == 1) {
				if (tabla[i].s_j1 == socket)
					encontrado = 1;
			} else if (j == 2) {
				if (tabla[i].s_j2 == socket)
					encontrado = 1;
			} else if (j == 3) {
				if (tabla[i].s_j3 == socket)
					encontrado = 1;
			} else if (j == 4) {
				if (tabla[i].s_j4 == socket)
					encontrado = 1;
			}
		}
		if (!encontrado)
			i++;
	}
	if (encontrado)
		return i;
	else
		return -1;
}

int EliminarJugadorPartida (TablaPartidas tabla, int ID, int socket) {
	//Elimina de la partida, ID, al jugador cuyo socket recibe como par�metro.
	if (tabla[ID].s_j1 == socket) {
		strcpy(tabla[ID].u_j1, "");
		tabla[ID].s_j1 = -1;
	} else if (tabla[ID].s_j2 == socket) {
		strcpy(tabla[ID].u_j2, "");
		tabla[ID].s_j2 = -1;
	} else if (tabla[ID].s_j3 == socket) {
		strcpy(tabla[ID].u_j3, "");
		tabla[ID].s_j3 = -1;
	} else if (tabla[ID].s_j4 == socket) {
		strcpy(tabla[ID].u_j4, "");
		tabla[ID].s_j4 = -1;
	}
	return 0;
}

int EliminarPartidaPorID(TablaPartidas tabla, int ID) {
	//Elimina la partida cuyo ID recibe como par�metro.
	strcpy(tabla[ID].u_j1, "");
	tabla[ID].s_j1 = -1;
	strcpy(tabla[ID].u_j2, "");
	tabla[ID].s_j2 = -1;
	strcpy(tabla[ID].u_j3, "");
	tabla[ID].s_j3 = -1;
	strcpy(tabla[ID].u_j4, "");
	tabla[ID].s_j4 = -1;
	return 0;
}

int EliminarPartidaPorJugador (TablaPartidas tabla, char jugador [20]) {
	//Elimina la partida recibiendo el nombre de un jugador como par�metro.
	int pos = BuscarPosicionPartidaPorJugador (tabla, jugador);
	if (pos == -1)
		return -1;
	else {
		strcpy(tabla[pos].u_j1, "");
		tabla[pos].s_j1 = -1;
		strcpy(tabla[pos].u_j2, "");
		tabla[pos].s_j2 = -1;
		strcpy(tabla[pos].u_j3, "");
		tabla[pos].s_j3 = -1;
		strcpy(tabla[pos].u_j4, "");
		tabla[pos].s_j4 = -1;
		return 0;
	}
}

int EliminarPartidaPorSocket (TablaPartidas tabla, int socket) {
	//Elimina la partida cuyo socket recibe como par�metro.
	int pos = BuscarPosicionPartidaPorSocket (tabla, socket);
	if (pos == -1)
		return -1;
	else {
		strcpy(tabla[pos].u_j1, "");
		tabla[pos].s_j1 = -1;
		strcpy(tabla[pos].u_j2, "");
		tabla[pos].s_j2 = -1;
		strcpy(tabla[pos].u_j3, "");
		tabla[pos].s_j3 = -1;
		strcpy(tabla[pos].u_j4, "");
		tabla[pos].s_j4 = -1;
		return 0;
	}
}

void DameJugadores (TablaPartidas tabla, int IDpartida, char jugadores[300]) {
	//Devuelve el n�mero de jugadores y sus nombres separador por comas, con el ID de la partida como par�metro.
	int count = 1;
	if (tabla[IDpartida].s_j2 != -1)
		count++;
	if (tabla[IDpartida].s_j3 != -1)
		count++;
	if (tabla[IDpartida].s_j4 != -1)
		count++;
	sprintf (jugadores, "%d,%s", count, tabla[IDpartida].u_j1);
	if (count >= 2)
		sprintf (jugadores, "%s,%s", jugadores, tabla[IDpartida].u_j2);
	if (count >= 3)
		sprintf (jugadores, "%s,%s", jugadores, tabla[IDpartida].u_j3);
	if (count == 4)
		sprintf (jugadores, "%s,%s", jugadores, tabla[IDpartida].u_j4);
	printf("Funci�nDameJugadores: %s\n", jugadores);
}

int i=0;

//Estructura necesaria para acceso excluyente
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *AtenderCliente (void *socket) {
	int sock_conn = * (int *) socket;
	printf ("Socket: %d\n", sock_conn);
	
	char peticion[512];
	char respuesta[512];
	int ret;
	
	//MYSQL
	MYSQL *conn;
	int err;
	//Creamos una conexion al servidor MYSQL 
	conn = mysql_init(NULL);
	if (conn==NULL) {
		printf ("Error al crear la conexion: %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//inicializar la conexion
	//conn = mysql_real_connect (conn, "localhost","root", "mysql", "M3_BD",0, NULL, 0); //Entorno Desarrollo (tambi�n quitar librer�a)
	conn = mysql_real_connect (conn, "shiva2.upc.es","root", "mysql", "M3_BD",0, NULL, 0); //Entorno Producci�n
	if (conn==NULL) {
		printf ("Error al inicializar la conexion: %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	int terminar =0;
	// Entramos en un bucle para atender todas las peticiones de este cliente
	//hasta que se desconecte
	while (terminar ==0)
	{
		// Ahora recibimos la peticion
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf ("Recibido\n");
		// Tenemos que a�adirle la marca de fin de string 
		// para que no escriba lo que hay despues en el buffer
		peticion[ret]='\0';
		printf ("Peticion: %s\n", peticion);
		// vamos a ver que quieren
		char *p = strtok( peticion, "/");
		int codigo =  atoi (p);
		// Ya tenemos el codigo de la peticion
		printf ("Codigo: %d\n", codigo);
		int numForm;
		//No corta en los c�digo que no a�aden m�s par�metros
		if ((codigo !=0)||(codigo !=14)||(codigo !=15))
			p = strtok( NULL, "/");
		//Saca en numero de form 2, para las consultas desde y hacia all�
		if ((codigo==11)||(codigo==12)||(codigo==13)) {
			numForm =  atoi (p);
			printf ("Form Secundario: %d\n", numForm);
			p = strtok( NULL, "/");
		}	
		//PAR�METROS QUE RECIBIMOS DEL CLIENTE O LE ENVIAMOS
		char usuario[20];
		char contrasena[20];
		char adversario[20];
		char min[50];
		char max[50];
		char InvitadosPartida[300];
		char RespuestaInvitados[10];
		char jugadores[300];
		char MensajeJugador[100];
		char JugadaJugador[100];
		char turnoAct[20];
		int IDpartida;
		int IDganadorDentro;
		
		if (codigo ==0) { //Peticion de Desconexion
			Desconectar(sock_conn);
			terminar=1;
		} else if (codigo ==1) { //Peticion de Registro
			strcpy (usuario, p);
			p = strtok( NULL, "/");
			strcpy (contrasena, p);
			Registrarse(usuario,contrasena,respuesta,conn,sock_conn);
		} else if (codigo ==2) { //Peticion de Login
			strcpy (usuario, p);
			p = strtok( NULL, "/");
			strcpy (contrasena, p);
			int cred = Entrar(usuario,contrasena,respuesta,conn,sock_conn);
			if (cred == 0)
				CredencialesCorrectas(usuario,respuesta,sock_conn);
		} else if (codigo ==14) { //Peticion de Salir
			Salir(respuesta, sock_conn);
		} else if (codigo ==15) { //Peticion de EliminarCuenta
			EliminarCuenta(respuesta,conn,sock_conn);
		} else if (codigo ==3) { //Peticion de Contrase�a
			strcpy (usuario, p);
			Contrasena(usuario,respuesta,conn,sock_conn);
		} else if (codigo ==4) { //Peticion de JugadoresBDPorID
			IDpartida = atoi (p);
			JugadoresBDPorID(IDpartida,respuesta,conn,sock_conn);
		} else if (codigo ==16) { //Peticion de JugadoresBDContra
			strcpy (usuario, p);
			JugadoresBDContra(usuario,respuesta,conn,sock_conn);
		} else if (codigo ==5) { //Peticion de Ganador
			IDpartida = atoi (p);
			Ganador(IDpartida,respuesta,conn,sock_conn);
		} else if (codigo ==6) { //Peticion de Tiempo
			IDpartida = atoi (p);
			Tiempo(IDpartida,respuesta,conn,sock_conn);
		} else if (codigo ==18) { //Peticion de IntervaloFecha
			strcpy (min, p);
			p = strtok( NULL, "/");
			strcpy (max, p);
			IntervaloFecha(min,max,respuesta,conn,sock_conn);
		} else if (codigo ==7) //Peticion de ganador m�s r�pido
			Rapido(respuesta,conn,sock_conn);
		else if (codigo ==17) { //Peticion de GanadorAmbosEnPartida
			strcpy (usuario, p);
			p = strtok( NULL, "/");
			strcpy (adversario, p);
			GanadorAmbosEnPartida(adversario,usuario,respuesta,conn,sock_conn);
		} //8 reservado para respuesta a Notificaci�n de Lista Conectados
		else if (codigo ==9) { //Notificaci�n de Invitar
			strcpy (InvitadosPartida, p);
			Invitar(InvitadosPartida,sock_conn);
		} else if (codigo ==10) { //Notificaci�n de Respuesta Invitaci�n
			IDpartida = atoi (p);
			p = strtok( NULL, "/");
			strcpy (RespuestaInvitados, p);
			RespuestaInvitacion(RespuestaInvitados,IDpartida,sock_conn,jugadores,conn);
		} else if (codigo ==11) { //Notificaci�n de Jugada
			IDpartida = atoi (p);
			p = strtok( NULL, "/");
			strcpy (turnoAct, p);
			p = strtok( NULL, "/");
			strcpy (JugadaJugador, p);
			p = strtok( NULL, "/");
			IDganadorDentro = atoi (p);
			Jugada(JugadaJugador,turnoAct,IDpartida,IDganadorDentro,sock_conn,numForm,jugadores,conn);
		} else if (codigo ==12) { //Notificaci�n de Mesaje
			IDpartida = atoi (p);
			p = strtok( NULL, "/");
			strcpy (MensajeJugador, p);
			Mensaje(MensajeJugador,IDpartida,sock_conn,numForm);
		} else if (codigo ==13) { //Notificaci�n de Abandono partida
			IDpartida = atoi (p);
			p = strtok( NULL, "/");
			strcpy (usuario, p);
			AbandonoPartida(usuario,IDpartida,sock_conn,numForm,jugadores,conn);
		} else
			strcpy (respuesta,"C�digo de petici�n no v�lido");

		if ((codigo==1)||(codigo==2)||(codigo==3)||(codigo==4)||(codigo==5)||(codigo==6)||(codigo==7)||(codigo==14)||(codigo==15)||(codigo==16)||(codigo==17)||(codigo==18)) //Enviamos respuesta
			write (sock_conn,respuesta, strlen(respuesta));
		if ((codigo ==2)||(codigo==0)||(codigo==14)||(codigo==15)) //Notificaci�n de Lista Conectados
			Conectados (respuesta);
	}
	mysql_close (conn); //Cerramos MYSQL
	close(sock_conn); //Se acabo el servicio para este cliente
}

int Desconectar (int socket) {
	//Borramos el cliente de la lista y cerramos conexion
	pthread_mutex_lock(&mutex); //No me interrumpas ahora
	int res = EliminaPorSocket(&lista,socket);
	pthread_mutex_unlock(&mutex); //ya puedes interrumpirme
	if (res == 0)
		printf ("Se ha eliminado el usuario\n");
	else
		printf ("No se hab�a iniciado sesi�n\n");
	return 0;
}

int Registrarse (char usuario[20],char contrasena[20], char respuesta[100],MYSQL *conn,int sock_conn){
	//Funcion para registrarse
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	//Limitamos la longitud de car�cteres del nombre de usuario para que no peten otras consultas
	if (strlen(usuario) > 20) {
		printf("El nombre de usuario es muy largo\n");
		strcpy(respuesta, "1/-2");
		return -2;
	} else {
		char consulta[100];
		sprintf (consulta, "SELECT Jugador.Usuario FROM Jugador WHERE Jugador.Usuario = '%s'", usuario);
		//Consulta: Busca si existe el usuario
		int err=mysql_query (conn, consulta);
		if (err!=0) {
			printf ("Error al consultar datos de la base %u %s\n",
					mysql_errno(conn), mysql_error(conn));
			exit (1);
		}
		//recogemos el resultado de la consulta 
		resultado = mysql_store_result (conn);
		row = mysql_fetch_row (resultado);
		if (row == NULL)
		{
			printf ("El usuario no existe\n");
			err=mysql_query (conn, "SELECT MAX(Jugador.ID) FROM Jugador");
			if (err!=0) {
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				exit (1);
				//recogemos el resultado de la consulta 
			}
			resultado = mysql_store_result (conn);
			row = mysql_fetch_row (resultado);
			if (row == NULL)
				printf ("No se han obtenido datos en la consulta\n");
			else {
				int ID = atoi (row[0]) +1;
				printf("%d\n", ID);
				// Ahora construimos el string con el comando SQL para insertar la persona en la base
				sprintf (consulta, "INSERT INTO Jugador VALUES (%d,'%s','%s')", ID, usuario, contrasena);
				printf("%s\n", consulta);
				// Ahora ya podemos realizar la insercion 
				err = mysql_query(conn, consulta);
				if (err!=0) {
					printf ("Error al introducir datos la base %u %s\n", 
							mysql_errno(conn), mysql_error(conn));
					exit (1);
				} else {
					printf("%s se ha registrado correctamente\n", usuario);
					strcpy(respuesta, "1/0");
					return 0;
				}
			}
		} else {
			printf("El usuario %s ya existe\n", row[0]);
			strcpy(respuesta, "1/-1");
			return -1;
		}	
	}
	printf("%s\n", respuesta);
}
int Entrar (char usuario[20],char contrasena[20], char respuesta[100],MYSQL *conn,int sock_conn) {
	//Funcion para entrar
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[200];
	sprintf (consulta, "SELECT Jugador.Usuario FROM Jugador WHERE Jugador.Usuario = '%s' AND Jugador.Contrasena = '%s'", usuario, contrasena);
	//Consulta: Busca si existe el usuario y la contrase�a
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL)
	{
		printf ("Datos de acceso inv�lidos\n");
		strcpy(respuesta, "2/-3");
		printf("%s\n", respuesta);
		return -3;
	} else {
		printf ("Datos de acceso correctos\n");
		//strcpy(respuesta, "2/0"); //Siempre lo va a chafar CredencialesCorrectas, se corrige en el cliente
		return 0;
	}
}
int CredencialesCorrectas (char usuario[20], char respuesta[100], int sock_conn) {
	//Revisar si las credenciales son correctas, cuando se solicita Entrar.
	int pos = DamePosicionPorSocket (&lista, sock_conn);
	if (pos != -1) {
		printf("En este cliente ya se hab�a iniciado sesi�n con otro usuario.\n");
		strcpy(respuesta, "2/-2");
		return -2;
	} else {
		pos = DamePosicion (&lista, usuario);
		if (pos == -1) {
			pthread_mutex_lock(&mutex);
			int pon = Pon (&lista,usuario,sock_conn);
			pthread_mutex_unlock(&mutex);
			if (pon == -1) {
				printf("No se ha podido iniciar sesi�n, la lista de conectados est� llena y no se ha podido a�adir.\n");
				strcpy(respuesta, "2/-1");
				return -1;
			} else{
				printf("%s ha iniciado sesi�n y se ha a�adido a la lista de conectados\n", usuario);
				strcpy(respuesta, "2/1");
				return 1;
			}
		} else {
			if (lista.conectados[pos].socket == sock_conn) {
				printf("%s ya hab�a iniciado sesi�n en este cliente y est� en la lista de conectados\n", usuario);
				strcpy(respuesta, "2/2");
				return 2;
			} else {
				printf("%s ya hab�a iniciado sesi�n en otro cliente y est� en la lista de conectados\n", usuario);
				strcpy(respuesta, "2/3");
				return 3;
			}
		}
	}
	printf("%s\n", respuesta);
}
int Salir (char respuesta[100],int sock_conn){
	//Funci�n para Salir
	//Borramos el cliente de la lista
	printf ("Socket: %d\n", sock_conn); //Debug
	pthread_mutex_lock(&mutex); //No me interrumpas ahora
	int res = EliminaPorSocket(&lista,sock_conn);
	pthread_mutex_unlock(&mutex); //ya puedes interrumpirme
	if (res == 0) {
		printf ("Se ha cerrado sesi�n\n");
		strcpy(respuesta, "14/1");
		return 1;
	} else if (res == -1) {
		printf ("No se hab�a iniciado sesi�n\n");
		strcpy(respuesta, "14/-1");
		return -1;
	}
}
int EliminarCuenta (char respuesta[100],MYSQL *conn,int sock_conn){
	//Funci�n para EliminarCuenta de la BD
	int pos = DamePosicionPorSocket(&lista, sock_conn);
	if (pos == -1) {
		printf ("No se hab�a iniciado sesi�n\n");
		strcpy(respuesta, "15/-1");
		return -1;
	} else {
		char usuario[20];
		strcpy(usuario, lista.conectados[pos].nombre);
		MYSQL_RES *resultado;
		MYSQL_ROW row;
		char consulta[100];
		sprintf (consulta, "DELETE FROM Jugador WHERE Jugador.Usuario = '%s'", usuario);
		//Consulta: Eliminar Jugador de la BD a partir de su Usuario
		int err=mysql_query (conn, consulta);
		if (err!=0) {
			printf ("Error al consultar datos de la base %u %s\n",
					mysql_errno(conn), mysql_error(conn));
			strcpy(respuesta, "15/-2");
			exit (1);
		} else {
			//Cerramos sesi�n
			int res = Salir(respuesta, sock_conn);
			if (res == 1) {
				printf ("Se ha cerrado sesi�n y eliminado el usuario\n");
				strcpy(respuesta, "15/1");
				return 1;
			} else if (res == -1) {
				printf ("Se ha eliminado el usuario, pero no cerrar sesi�n\n");
				strcpy(respuesta, "15/-3");
				return -3;
			}
		}
	}
}


void Contrasena (char usuario[20], char respuesta[100],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver la contrasena de la BD a partir del nombre de usuario
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[100];
	sprintf (consulta, "SELECT Jugador.Contrasena FROM Jugador WHERE Jugador.Usuario = '%s'", usuario);
	//Consulta: Buscar la contrase�a a partir del nombre de usuario
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "3/fail");
	} else {
		sprintf(respuesta, "3/%s", row[0]);
		printf("%s\n", respuesta);
	}
}
void JugadoresBDPorID (int partida, char respuesta[100],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver los jugadores de una partida, de la BD
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[200];
	sprintf (consulta, "SELECT Jugador.Usuario FROM (Jugador, Partida, Jugadores) WHERE Partida.ID = '%d' AND Jugador.ID = Jugadores.ID_J AND Partida.ID = Jugadores.ID_P", partida);
	//Consulta: Buscar los jugadores de una partida a partir del identificador
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "4/fail");
	} else {
		strcpy(respuesta, "4/");
		while (row !=NULL) {
			printf("%s\n", row[0]);
			sprintf(respuesta, "%s%s/", respuesta, row[0]);
			row = mysql_fetch_row (resultado);
		}
		printf("%s\n", respuesta);
	}
}
void JugadoresBDContra (char usuario[20], char respuesta[100],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver los jugadores contra los que he jugado, de la BD
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[900];
	sprintf (consulta, "SELECT DISTINCT Jugador.Usuario FROM (Jugador, Partida, Jugadores) WHERE Partida.ID IN (SELECT Partida.ID FROM (Jugador, Partida, Jugadores) WHERE Jugador.Usuario = '%s' AND Jugador.ID = Jugadores.ID_J AND Partida.ID = Jugadores.ID_P) AND Jugador.ID = Jugadores.ID_J AND Partida.ID = Jugadores.ID_P AND Jugador.Usuario != '%s'", usuario, usuario);
	//Consulta: Buscar los jugadores de una partida a partir del identificador
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "16/fail");
	} else {
		strcpy(respuesta, "16/");
		while (row !=NULL) {
			printf("%s\n", row[0]);
			sprintf(respuesta, "%s%s/", respuesta, row[0]);
			row = mysql_fetch_row (resultado);
		}
		printf("%s\n", respuesta);
	}
}
void Ganador (int partida, char respuesta[100],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver el ganador de una partida cuyo ID se recibe como parametro, en la BD
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[100];
	sprintf (consulta, "SELECT Partida.Ganador FROM Partida WHERE Partida.ID = '%d'", partida);
	//Consulta: Buscar el ganador de una partida a partir del identificador
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "5/fail");
	} else {
		sprintf(respuesta, "5/%s", row[0]);
		printf("%s\n", respuesta);
	}
}
void Tiempo (int partida, char respuesta[100],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver la duraci�n de una partida cuyo ID se recibe como parametro, de la BD
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[100];
	sprintf (consulta, "SELECT Partida.Tiempo FROM Partida WHERE Partida.ID = '%d'", partida);
	//Consulta: Buscar la duraci�n de una partida a partir del identificador
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "6/fail");
	} else {
		int tiempo = atoi (row[0]);
		sprintf(respuesta, "6/%s", row[0]);
		printf("%s\n", respuesta);
	}
}
void IntervaloFecha (char min[50], char max[50], char respuesta[100],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver las partidas en un intervalo de tiempo, de la BD
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[100];
	sprintf (consulta, "SELECT Partida.ID, Partida.Fecha FROM Partida");
	//Consulta: Buscar la duraci�n de una partida a partir del identificador
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "18/fail");
	} else {
		strcpy(respuesta, "18/");
		//Fecha inicio
		char *p = strtok( min, "-");
		int DiaMin = atoi(p);
		p = strtok( NULL, "-");
		int MesMin = atoi(p);
		p = strtok( NULL, " ");
		int AnoMin = atoi(p);
		p = strtok( NULL, ":");
		int HoraMin = atoi(p);
		p = strtok( NULL, "");
		int MinutoMin = atoi(p);
		//Fecha fin
		p = strtok( max, "-");
		int DiaMax = atoi(p);
		p = strtok( NULL, "-");
		int MesMax = atoi(p);
		p = strtok( NULL, " ");
		int AnoMax = atoi(p);
		p = strtok( NULL, ":");
		int HoraMax = atoi(p);
		p = strtok( NULL, "");
		int MinutoMax = atoi(p);
		//Fecha BD
		char Fecha[100];
		strcpy(Fecha,row[1]);
		while (row !=NULL) {
			//Fecha partida
			int ID = atoi(row[0]);
			char *p = strtok( row[1], "-");
			int Dia = atoi(p);
			p = strtok( NULL, "-");
			int Mes = atoi(p);
			p = strtok( NULL, " ");
			int Ano = atoi(p);
			p = strtok( NULL, ":");
			int Hora = atoi(p);
			p = strtok( NULL, "");
			int Minuto = atoi(p);
			if ((AnoMin <= Ano) && (Ano <= AnoMax))
				if ((MesMin <= Mes) && (Mes <= MesMax))
					if ((DiaMin <= Dia) && (Dia <= DiaMax))
						if ((HoraMin <= Hora) && (Hora <= HoraMax))
							if ((MinutoMin <= Minuto) && (Minuto <= MinutoMax))
								sprintf(respuesta, "%s%d/%s/", respuesta, ID, Fecha);
			row = mysql_fetch_row (resultado);
		}
		printf("%s\n", respuesta);
	}
}
void Rapido (char respuesta[100],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver el ganador mas r�pido de una partida
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	//Consulta: Buscar la jugador que ha ganada m�s r�pido
	int err=mysql_query (conn, "SELECT Partida.Ganador FROM Partida WHERE Partida.Tiempo = (SELECT MIN(Partida.Tiempo) FROM Partida)");
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "7/fail");
	} else {
		sprintf(respuesta, "7/%s", row[0]);
		printf("%s\n", respuesta);
	}
}

void GanadorAmbosEnPartida (char adversario[20], char usuario[20], char respuesta[512],MYSQL *conn,int sock_conn){
	//Procedimiento para devolver los ganadores de las partidas en los que ha jugado tanto el usuario como un determinado adversario, de la BD
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[900];
	sprintf (consulta, "SELECT DISTINCT Partida.ID, Partida.Ganador FROM (Jugador, Partida, Jugadores) WHERE Jugador.Usuario = '%s' AND Partida.ID IN (SELECT Partida.ID FROM (Jugador, Partida, Jugadores) WHERE Jugador.Usuario = '%s' AND Jugador.ID = Jugadores.ID_J AND Partida.ID = Jugadores.ID_P) AND Jugador.ID = Jugadores.ID_J AND Partida.ID = Jugadores.ID_P",
			 adversario, usuario);
	//Consulta: Buscar los jugadores de una partida a partir del identificador
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL) {
		printf ("No se han obtenido datos en la consulta\n");
		strcpy(respuesta, "17/fail");
	} else {
		strcpy(respuesta, "17/");
		while (row !=NULL) {
			int ID = atoi(row[0]);
			sprintf(respuesta, "%s%d/%s/", respuesta, ID, row[1]);
			row = mysql_fetch_row (resultado);
		}
		printf("%s\n", respuesta);
	}
}

void Conectados (char respuesta[512]) {
	//Enviamos la Lista de Conectados como notificaci�n
	DameConectados(&lista, respuesta);
	char notificacion[512];
	sprintf (notificacion, "8/%s", respuesta);
	printf("Conectados: %s\n",notificacion);
	for (int j=0; j < lista.num; j++)
		write (lista.conectados[j].socket, notificacion, strlen(notificacion));
}

void Invitar (char InvitadosPartida[300],int socket) {
	//Si se cumplen las condiciones, env�a las invitaciones, si no, notifica al anfitri�n
	char notificacion[512];
	printf ("InvitadosPartida: %s\n", InvitadosPartida); //Debug
	pthread_mutex_lock(&mutex);
	int res = CrearPartida(&lista,InvitadosPartida,tabla, socket);
	pthread_mutex_unlock(&mutex);
	printf ("res de CrearPartida: %d\n", res); //Debug
	if (res == -1) {
		printf ("No se ha encontrado a un invitado\n");
		strcpy(notificacion,"9/-1");
		write (socket, notificacion, strlen(notificacion));
	} else if (res == -2) {
		printf ("No hay jugadores suficientes\n");
		strcpy(notificacion,"9/-2");
		write (socket, notificacion, strlen(notificacion));
	} else if (res == -3) {
		printf ("No se permiten m�s partidas\n");
		strcpy(notificacion,"9/-3");
		write (socket, notificacion, strlen(notificacion));
	} else {
		sprintf(notificacion,"9/%s/%d",tabla[res].u_j1,res);
		printf ("Notificaci�n: %s\n", notificacion);
		write (tabla[res].s_j2, notificacion, strlen(notificacion));
		if ((tabla[res].u_j3 != "") || (tabla[res].s_j3 != -1))
			write (tabla[res].s_j3, notificacion, strlen(notificacion));
		if ((tabla[res].u_j4 != "") || (tabla[res].s_j4 != -1))
			write (tabla[res].s_j4, notificacion, strlen(notificacion));
		return 0;
	}
}

void RespuestaInvitacion (char RespuestaInvitados[10],int IDpartida,int socket, char jugadores[300]) {
	//Notifica a los participantes si la partida se inicia o no
	char notificacion[512];
	int res =0;
	respuestasinvitaciones++;
	if (invitados == 0) {
		if (tabla[IDpartida].s_j2 != -1)
			invitados++;
		if (tabla[IDpartida].s_j3 != -1)
			invitados++;
		if (tabla[IDpartida].s_j4 != -1)
			invitados++;
	}
	printf("Invitados: %d\n", invitados);
	printf("respuestasinvitaciones: %d\n", respuestasinvitaciones);
	if (strcmp(RespuestaInvitados, "NO") ==0) {
		pthread_mutex_lock(&mutex);
		res = EliminarJugadorPartida(tabla,IDpartida,socket);
		pthread_mutex_unlock(&mutex);
		if (res != 0)
			printf("Error al eliminar un invitado\n");
		else {
			printf("Invitado eliminado correctamente\n");
			invitadoseliminados++;
			printf ("Resultado: %s/%d/%s/%d/%s/%d/%s/%d\n", tabla[IDpartida].u_j1,tabla[IDpartida].s_j1,
					tabla[IDpartida].u_j2,tabla[IDpartida].s_j2,tabla[IDpartida].u_j3,tabla[IDpartida].s_j3,
					tabla[IDpartida].u_j4, tabla[IDpartida].s_j4); //Debug
		}
	}
	printf("Invitadoseliminados: %d\n", invitadoseliminados);
	if (respuestasinvitaciones == invitados) {
		int op = invitados-invitadoseliminados;
		printf("op: %d\n", op);
		if (op > 0) {
			DameJugadores(tabla,IDpartida,jugadores);
			printf("Jugadores: %s\n", jugadores);
			sprintf(notificacion,"10/%d/TRUE/%s",IDpartida,jugadores);
		} else
			sprintf(notificacion,"10/%d/FALSE",IDpartida);
		
		printf("Notificacion: %s\n", notificacion);
		
		if (tabla[IDpartida].s_j1 != -1)
			write (tabla[IDpartida].s_j1, notificacion, strlen(notificacion));
		if (tabla[IDpartida].s_j2 != -1)
			write (tabla[IDpartida].s_j2, notificacion, strlen(notificacion));
		if (tabla[IDpartida].s_j3 != -1)
			write (tabla[IDpartida].s_j3, notificacion, strlen(notificacion));
		if (tabla[IDpartida].s_j4 != -1)
			write (tabla[IDpartida].s_j4, notificacion, strlen(notificacion));
		invitados = 0;
		respuestasinvitaciones = 0;
	}
}

void Jugada (char JugadaJugador[100], char turnoAct[20], int IDpartida, int IDganadorDentro, int socket, int numForm, char jugadores[300], MYSQL *conn) {
	//Enviamos la Jugada como notificaci�n, tambien si hay, la partida se acaba y se guarda
	printf("JugadaJugador: %s\n",JugadaJugador); //Debug
	printf("turnoAct: %s\n",turnoAct); //Debug
	printf("SocketRecibido: %d\n",socket); //Debug
	printf("IDganadorDentro: %d\n",IDganadorDentro); //Debug
	char notificacion[512];
	sprintf (notificacion, "11/%d/%d/%s,%s,%d", numForm, IDpartida, turnoAct, JugadaJugador, IDganadorDentro);
	if (tabla[IDpartida].s_j1 != socket)
		write (tabla[IDpartida].s_j1, notificacion, strlen(notificacion));
	if (tabla[IDpartida].s_j2 != socket)
		write (tabla[IDpartida].s_j2, notificacion, strlen(notificacion));
	if (tabla[IDpartida].s_j3 != socket)
		write (tabla[IDpartida].s_j3, notificacion, strlen(notificacion));
	if (tabla[IDpartida].s_j4 != socket)
		write (tabla[IDpartida].s_j4, notificacion, strlen(notificacion));
	if (IDganadorDentro != -1) {
		char usuario[20];
		UsuarioPorIDjugadorDentro (IDganadorDentro,IDpartida,usuario);
		int res = GuardarPartidaBD(IDpartida,usuario,jugadores,conn);
		if (res == 0) {
			pthread_mutex_lock(&mutex);
			EliminarPartidaPorID(tabla,IDpartida);
			pthread_mutex_unlock(&mutex);
			printf("Partida eliminada\n");
		} else
			printf("No se ha podido guardar la partida en la BD\n");
	}
}

void Mensaje (char MensajeJugador[100], int IDpartida, int socket, int numForm) {
	//Enviamos en Mensaje como notificaci�n
	printf("MensajeJugador: %s\n",MensajeJugador); //Debug
	printf("SocketRecibido: %d\n",socket); //Debug
	char notificacion[512];
	sprintf (notificacion, "12/%d/%d/%s", numForm, IDpartida, MensajeJugador);
	if (tabla[IDpartida].s_j1 != socket) {
		write (tabla[IDpartida].s_j1, notificacion, strlen(notificacion));
		printf("SocketJ1: %d\n",tabla[IDpartida].s_j1); //Debug
		printf("Enviando a J1\n"); //Debug
	}
	if (tabla[IDpartida].s_j2 != socket) {
		write (tabla[IDpartida].s_j2, notificacion, strlen(notificacion));
		printf("SocketJ2: %d\n",tabla[IDpartida].s_j2); //Debug
		printf("Enviando a J2\n"); //Debug
	}
	if (tabla[IDpartida].s_j3 != socket)
		write (tabla[IDpartida].s_j3, notificacion, strlen(notificacion));
	if (tabla[IDpartida].s_j4 != socket)
		write (tabla[IDpartida].s_j4, notificacion, strlen(notificacion));
}

void AbandonoPartida (char usuario[20], int IDpartida, int socket, int numForm, char jugadores[300], MYSQL *conn) {
	//Enviamos el Abandono Partida como notificaci�n, la partida se acaba y se guarda
	printf("Jugador que ha abadonado: %s\n",usuario); //Debug
	printf("SocketRecibido: %d\n",socket); //Debug
	char notificacion[512];
	sprintf (notificacion, "13/%d/%d/%s", numForm, IDpartida, usuario);
	if (tabla[IDpartida].s_j1 != socket)
		write (tabla[IDpartida].s_j1, notificacion, strlen(notificacion));
	if (tabla[IDpartida].s_j2 != socket)
		write (tabla[IDpartida].s_j2, notificacion, strlen(notificacion));
	if (tabla[IDpartida].s_j3 != socket)
		write (tabla[IDpartida].s_j3, notificacion, strlen(notificacion));
	if (tabla[IDpartida].s_j4 != socket)
		write (tabla[IDpartida].s_j4, notificacion, strlen(notificacion));
	int res = GuardarPartidaBD(IDpartida,"Empate",jugadores,conn);
	if (res == 0) {
		pthread_mutex_lock(&mutex);
		EliminarPartidaPorID(tabla,IDpartida);
		pthread_mutex_unlock(&mutex);
		printf("Partida eliminada\n");
	} else
		printf("No se ha podido guardar la partida en la BD\n");
}

void UsuarioPorIDjugadorDentro (int IDjugadorPartida, int IDpartida, char usuario[20]) {
	if (IDjugadorPartida == 1)
		strcpy(usuario, tabla[IDpartida].u_j1);
	else if (IDjugadorPartida == 2)
		strcpy(usuario, tabla[IDpartida].u_j2);
	else if (IDjugadorPartida == 3)
		strcpy(usuario, tabla[IDpartida].u_j3);
	else if (IDjugadorPartida == 4)
		strcpy(usuario, tabla[IDpartida].u_j4);
}

int GuardarPartidaBD (int IDpartida, char ganador[20], char jugadores[300], MYSQL *conn) {
	//Funcion para Guardar una partida, en la BD
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta[500];
	sprintf (consulta, "SELECT MAX(Partida.ID) FROM Partida");
	//Consulta: Busca un ID de partida disponible
	int err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//recogemos el resultado de la consulta 
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	if (row == NULL)
		printf ("No se han obtenido datos en la consulta\n");
	else {
		int ID = atoi (row[0]) +1;
		printf("%d\n", ID);
		char fecha[150];
		time_t tiempo = time(0);
		struct tm *tlocal=localtime(&tiempo);
		strftime(fecha,150,"%d-%m-20%y %H:%M",tlocal);
		//Ahora construimos el string con el comando SQL para insertar la partida en la base
		sprintf (consulta, "INSERT INTO Partida VALUES (%d,'%s',NULL,'%s')", ID, fecha, ganador);
		printf("%s\n", consulta);
		//Ahora ya podemos realizar la insercion 
		err = mysql_query(conn, consulta);
		if (err!=0) {
			printf ("Error al introducir datos la base %u %s\n", 
					mysql_errno(conn), mysql_error(conn));
			exit (1);
		} else {
			printf("Se ha a�adido la PARTIDA correctamente\n");
			DameJugadores(tabla,IDpartida,jugadores);
			char *p = strtok( jugadores, ",");
			int numJug = atoi(p);
			printf("numJug: %d\n", numJug);
			p = strtok( NULL, ",");
			int cont = 0;
			while (p != NULL) {
				//Consulta: Buscar el ID a partir del nombre de usuario
				sprintf (consulta, "SELECT Jugador.ID FROM Jugador WHERE Jugador.Usuario = '%s'", p);
				int err=mysql_query (conn, consulta);
				if (err!=0) {
					printf ("Error al consultar datos de la base %u %s\n",
							mysql_errno(conn), mysql_error(conn));
					exit (1);
				}
				resultado = mysql_store_result (conn);
				row = mysql_fetch_row (resultado);
				if (row == NULL) {
					printf ("No se han obtenido datos en la consulta\n");
				} else {
					int IDjugadorBD = atoi(row[0]);
					printf("IDjugadorBD: %d\n", IDjugadorBD);
					//Ahora construimos el string con el comando SQL para relacionar las tablas
					sprintf (consulta, "INSERT INTO Jugadores VALUES (%d,%d)",IDjugadorBD, ID);
					printf("%s\n", consulta);
					//Ahora ya podemos realizar la insercion 
					err = mysql_query(conn, consulta);
					if (err!=0) {
						printf ("Error al introducir datos la base %u %s\n", 
								mysql_errno(conn), mysql_error(conn));
						exit (1);
					} else {
						printf("Se ha a�adido la relaci�n correctamente\n");
						p = strtok( NULL, ",");
						cont++;
					}
				}
				if (numJug == cont) {
					printf("Se ha a�adido la PARTIDA con sus relaciones correctamente\n");
					return 0;
				}
			}
		}
	}
}
	
//FUNCI�N PRINCIPAL
int main(int argc, char *argv[]) {
	int sock_conn, sock_listen;
	struct sockaddr_in serv_adr;

	// INICIALITZACIONS
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error creant socket\n");
	// Fem el bind al port
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;
	// asocia el socket a cualquiera de las IP de la m?quina. 
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	// establecemos el puerto de escucha
	//int puerto = 9036; //Entorno Desarrollo
	int puerto = 50057; //Entorno Producci�n
	serv_adr.sin_port = htons(puerto);
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0) {
		printf ("Error al bind\n");
		exit (1);
	}
	//La cola de peticiones pendientes no podr ser superior a 3
	if (listen(sock_listen, 3) < 0)
		printf("Error en el Listen\n");
	
	setvbuf (stdout, NULL, _IONBF,0);	
	pthread_mutex_lock(&mutex);
	InicializarTabla(tabla);
	pthread_mutex_unlock(&mutex);
	
	pthread_t thread;
	// Bucle infinito
	for (;;){
		printf ("Escuchando en el puerto %d\n", puerto);
		
		sock_conn = accept(sock_listen, NULL, NULL);
		printf ("He recibido conexion\n");
		
		lista.conectados[i].socket =sock_conn;
		//sock_conn es el socket que usaremos para este cliente
		
		// Crear thead y decirle lo que tiene que hacer
		pthread_create (&thread, NULL, AtenderCliente,&lista.conectados[i].socket);
		i++;
	}	
}
