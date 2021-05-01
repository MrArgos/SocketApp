
/*  Trabalho realizado por João Costa al59259, Luís Ribeiro al68708 e Pedro Monteiro al69605  */

#include<stdio.h>
#include<winsock2.h>
#include<string.h>

#pragma comment(lib,"ws2_32.lib") 
#pragma warning(disable : 4996)

int main(int argc, char* argv[])
{
	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;
	char* message = malloc(4096);
	char server_reply[4096];
	int connected = FALSE;
	int recv_size;
	int ws_result;
	char* ip = malloc(20);

	// Inicializar Winsocks
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}

	// Criar Socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}

	while (!connected) {

		// Pedir ip ao utilizador
		printf("\nPlease input Server IP Address> ");
		fgets(ip, 20, stdin);

		// Endereço IP e Porta do servidor
		server.sin_addr.s_addr = inet_addr(ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(68000);

		// Ligar ao servidor
		ws_result = connect(s, (struct sockaddr*)&server, sizeof(server));
		if (ws_result < 0)
		{
			puts("Error: Can't connect to Server.");
		}

		recv_size = recv(s, server_reply, 4096, 0);
		if (recv_size == SOCKET_ERROR)
		{
			puts("Error: No response from Server.");
		}

		if (strcmp(server_reply, "100 OK") == 0) {
			puts("Connected\n");
			connected = TRUE;
			puts("Commands:\n<GETKEY X> - Generates <X> Euromilhoes Keys.\n<QUIT> - Close connection and application.\n");
		}
	}

	while (TRUE)
	{
		ZeroMemory(message, 4096);
		ZeroMemory(server_reply, 4096);
		fputs("> ", stdout);
		fgets(message, 4096, stdin);

		ws_result = send(s, message, strlen(message) - 1, 0);
		if (ws_result < 0)
		{
			puts("Send failed");
			return 1;
		}

		recv_size = recv(s, server_reply, 4096, 0);
		if (recv_size == SOCKET_ERROR)
		{
			puts("Receive failed");
		}
		if (strcmp(server_reply, "200 SENDING") == 0) {
			ZeroMemory(server_reply, 4096);
			recv_size = recv(s, server_reply, 4096, 0);
			printf("Server> \n%s\n", server_reply);
		}
		else if (strcmp(server_reply, "300 UNRECOGNISED") == 0)
		{
			printf("Server> Error: Unrecognised command\n");
		}
		else if (strcmp(server_reply, "301 MISSING") == 0)
		{
			printf("Server> Error: <GETKEY X> - <X> missing (number of keys to generate).\n");
		}
		else if (strcmp(server_reply, "302 UNEXPECTED") == 0)
		{
			printf("Server> Error: <GETKEY X> - Expected a number after <GETKEY> command.\n");
		}
		else if (strcmp(server_reply, "400 BYE") == 0)
		{
			printf("Server> Closing connection... Bye!\n");
			system("pause");
			break;
		}
	}

	closesocket(s);
	WSACleanup();

	return 0;
}
