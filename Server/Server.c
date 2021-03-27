#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <time.h>
#include <Windows.h>
#define TRUE 1
#define DS_TEST_PORT 68000
#define STARS_SIZE 2
#define KEY_SIZE_NO_STARS 5
#define KEY_SIZE_FULL 7

#pragma comment (lib, "ws2_32.lib")
#pragma warning(disable : 4996)

void GenerateKey(int* key);
int cmp_fnc(const void* a, const void* b);
const char* StringFromKey(int* key);
void SaveToFile(char message[]);

int main()
{
	// Inicializar vari�veis para gera��o de chaves
	int key[KEY_SIZE_FULL];
	char message[2000] = "";
	const char* keyString;
	char* ptrToken;
	int numKeysInt;
	char numKeysChar[100];
	strcpy_s(message, 2000, "");
	srand((unsigned)time(NULL));

	// Initialise winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	//printf("\nInitialising Winsock...");
	int wsResult = WSAStartup(ver, &wsData);
	if (wsResult != 0) {
		fprintf(stderr, "\nWinsock setup fail! Error Code : %d\n", WSAGetLastError());
		return 1;
	}

	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		fprintf(stderr, "\nSocket creationg fail! Error Code : %d\n", WSAGetLastError());
		return 1;
	}

	printf("\nInitialized. Waiting for connection...\n");

	// Bind the socket (ip address and port)
	struct sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(DS_TEST_PORT);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (struct sockaddr*)&hint, sizeof(hint));

	// Setup the socket for listening
	listen(listening, SOMAXCONN);

	// Wait for connection
	struct sockaddr_in client;
	int clientSize = sizeof(client);

	SOCKET clientSocket = accept(listening, (struct sockaddr*)&client, &clientSize);

	// Close listening socket
	closesocket(listening);

	// Main program loop
	char strMsg[4096];
	char strRec[4096];

	int msgCount = 0;
	int numKeys = 0;

	strcpy(strMsg, "100 OK");
	send(clientSocket, strMsg, strlen(strMsg) + 1, 0);
	while (TRUE) {
		ZeroMemory(strRec, 4096);
		ZeroMemory(strMsg, 4096);
		int bytesReceived = recv(clientSocket, strRec, 4096, 0);
		if (bytesReceived == SOCKET_ERROR) {
			printf("\nReceive error!\n");
			break;
		}
		if (bytesReceived == 0) {
			printf("\nClient disconnected!\n");
			break;
		}

		if (strcmp(strRec, "\r\n") != 0)
		{

			printf("%i : %s\n", ++msgCount, strRec);

			if (strcmp(strRec, "QUIT") == 0) {
				strcpy(strMsg, "400 BYE");
				send(clientSocket, strMsg, strlen(strMsg) + 1, 0);
				printf("Response: %s\n", strMsg);
				break;
			}

			ptrToken = strtok(strRec, " ");
			if (strcmp(ptrToken, "GETKEY") == 0)
			{
				ptrToken = strtok(NULL, " ");
				if (ptrToken != NULL) {
					numKeys = atoi(ptrToken);
					if (numKeys) {
						strcpy(strMsg, "200 SENT");
						send(clientSocket, strMsg, strlen(strMsg) + 1, 0);
						ZeroMemory(strMsg, 4096);
						for (int i = 0; i < numKeys; i++)
						{
							GenerateKey(key);
							keyString = StringFromKey(key);
							strcat_s(strMsg, 4069, keyString);
							strcat_s(strMsg, 4096, "\r\n");
						}
					}
					else
					{
						strcpy(strMsg, "302 UNEXPECTED");
					}
				}
				else
				{
					strcpy(strMsg, "301 MISSING");
				}
			}
			else
			{
				strcpy(strMsg, "300 UNRECOGNISED");
			}
			send(clientSocket, strMsg, strlen(strMsg) + 1, 0);
			printf("Response: %s\n", strMsg);
		}
	}

	// Close the socket
	closesocket(clientSocket);

	//Cleanup winsock
	WSACleanup();

	return 1;
}

/// <summary>
/// Fun��o que gera chaves do Euromilh�es e guarda no array passado por par�metro.
/// Output: XX XX XX XX XX EE EE
/// </summary>
/// <param name="key">- array onde ser� guardada a chave</param>
void GenerateKey(int* key) {

	// Gerar chave - n�meros aleat�rios 1-50 - e guardar no array.  
	for (int i = 0; i < KEY_SIZE_NO_STARS; i++)
	{
		key[i] = rand() % 50 + 1;
		for (int j = 0; j < i; j++) // Verificar n�meros repetidos
		{
			if (key[i] == key[j])
			{
				i--;
				break;
			}
		}
	}

	// Ordenar array
	qsort(key, KEY_SIZE_NO_STARS, sizeof(int), cmp_fnc);

	// Gerar estrelas - n�meros 1-12
	int stars[STARS_SIZE];
	for (int i = 0; i < STARS_SIZE; i++)
	{
		stars[i] = rand() % 12 + 1;
		if ((i != 0) && (stars[i] == stars[i - 1]))
		{
			i--;
		}
	}
	// Ordenar estrelas
	if (stars[0] > stars[1])
	{
		key[5] = stars[1];
		key[6] = stars[0];
	}
	else
	{
		key[5] = stars[0];
		key[6] = stars[1];
	}
}

// Fun��o de compara��o a utilizar no qsort()
int cmp_fnc(const void* a, const void* b) {
	return (*(int*)a - *(int*)b);
}

/// <summary>
/// Retorna uma string com a chave passada � fun��o.
/// </summary>
/// <param name="key">Array que cont�m a chave ordenada</param>
/// <returns>string com a chave</returns>
const char* StringFromKey(int* key) {
	//char* str = (char*)malloc(2000 * sizeof(char));
	char* str = malloc(2000);
	if (str == NULL)
	{
		return "";
	}
	strcpy_s(str, 2000, "");

	// Escrever array com a chave numa string
	for (int i = 0; i < KEY_SIZE_FULL; i++)
	{
		if (key[i] < 10)
		{
			sprintf_s(&str[strlen(str)], (sizeof(int) + (3 * sizeof(char))), "%d   ", key[i]);
		}
		else
		{
			sprintf_s(&str[strlen(str)], (sizeof(int) + (3 * sizeof(char))), "%d  ", key[i]);
		}
		if (i == 4)
		{
			strcat_s(str, sizeof(char) * (strlen(str) + 7), " +   ");
		}
	}
	return str;
}

/// <summary>
/// Acrescenta a(s) chave(s) a um ficheiro.
/// </summary>
/// <param name="message">- array com a(s) chave(s) a guardar</param>
void SaveToFile(char message[])
{
	FILE* fp;
	errno_t erro;
	erro = fopen_s(&fp, "chaves.txt", "a");
	if ((fp != 0) || (erro != 0))
	{
		fprintf(stderr, "Erro a abrir ficheiro.\n");
		return;
	}
	if (fputs(message, fp) < 0)
	{
		fprintf(stderr, "Erro a escrever no ficheiro.\n");
		return;
	}
}
