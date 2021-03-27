/*
	Simple winsock client
*/

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

	// Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}

	//Create a socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}

	printf("Socket created.\n");

	while (!connected) {

		// Pedir ip ao utilizador
		printf("Insira IP do servidor> ");
		fgets(ip, 20, stdin);

		// create the socket  address (ip address and port)
		//server.sin_addr.s_addr = inet_addr("25.72.114.113");
		//server.sin_addr.s_addr = inet_addr("127.0.0.1");
		server.sin_addr.s_addr = inet_addr(ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(68000);

		//Connect to remote server
		ws_result = connect(s, (struct sockaddr*)&server, sizeof(server));
		if (ws_result < 0)
		{
			puts("connect error");
			return 1;
		}

		recv_size = recv(s, server_reply, 4096, 0);
		if (recv_size == SOCKET_ERROR)
		{
			puts("recv failed");
		}

		if (strcmp(server_reply, "100 OK") == 0) {
			puts("Connected\n");
			connected = TRUE;
			puts("Commands:\n<GETKEY X> - Generates <X> Euromilhoes Keys.\n<QUIT> - Closes connection and App.\n");
		}
	}

	while (TRUE)
	{
		ZeroMemory(message, 4096);
		ZeroMemory(server_reply, 4096);
		fputs("> ", stdout);
		fgets(message, 4096, stdin);

		ws_result = send(s, message, strlen(message) - 1, 0);  // (-1) para nao enviar \n
		if (ws_result < 0)
		{
			puts("Send failed");
			return 1;
		}

		recv_size = recv(s, server_reply, 4096, 0);
		if (recv_size == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		if (strcmp(server_reply, "200 SENT") == 0) {
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
			break;
		}
	}

	// Close the socket
	closesocket(s);

	//Cleanup winsock
	WSACleanup();

	return 0;
}
