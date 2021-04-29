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
void SaveStringToFile(char message[]);
DWORD WINAPI handleconnection(LPVOID lpParam);

int main()
{
	//int allKeys[100000][KEY_SIZE_FULL];

	/*int** allKeys = (int**)malloc(4096 * sizeof(int*));
	if (allKeys != NULL) {
		for (int i = 0; i < 4096; i++)
		{
			allKeys[i] = (int*)malloc(KEY_SIZE_FULL * sizeof(int));
		}
	}*/

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
	int clientSize;
	SOCKET clientSocket;
	SOCKET* ptclientSocket;
	DWORD dwThreadId;
	HANDLE hThread;
	int conresult = 0;

	while (TRUE)
	{
		clientSize = sizeof(client);
		clientSocket = accept(listening, (struct sockaddr*)&client, &clientSize);
		ptclientSocket = &clientSocket;

		//printf("\nNova conexao %s\n", client.sin_addr);

		hThread = CreateThread(NULL, 0, handleconnection, ptclientSocket, 0, &dwThreadId);

		if (hThread == NULL)
		{
			printf("\nErro de criacao de Thread.\n");
		}
	}

	closesocket(clientSocket);
	closesocket(listening);
	WSACleanup();
}

DWORD WINAPI handleconnection(LPVOID lpParam)
{
	// Main program loop
	char strMsg[4096];
	char strRec[4096];

	SOCKET cs;
	SOCKET* ptCs;
	ptCs = (SOCKET*)lpParam;
	cs = *ptCs;

	// variáveis para geração de chaves
	int key[KEY_SIZE_FULL];
	char message[2000] = "";
	const char* keyString;
	char* partialMsg;
	int numKeysInt;
	int countAllKeys = 0;
	int isAlike = 0;
	//char numKeysChar[100];
	strcpy_s(message, 2000, "");
	srand((unsigned)time(NULL));
	int msgCount = 0;
	int numKeys = 0;

	strcpy(strMsg, "100 OK");
	send(cs, strMsg, strlen(strMsg) + 1, 0);
	while (TRUE) {
		ZeroMemory(strRec, 4096);
		ZeroMemory(strMsg, 4096);
		int bytesReceived = recv(cs, strRec, 4096, 0);
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
				send(cs, strMsg, strlen(strMsg) + 1, 0);
				printf("Response: %s\n", strMsg);
				break;
			}

			partialMsg = strtok(strRec, " ");
			if (strcmp(partialMsg, "GETKEY") == 0)
			{
				partialMsg = strtok(NULL, " ");
				if (partialMsg != NULL) {
					numKeys = atoi(partialMsg);
					if (numKeys) {
						strcpy(strMsg, "200 SENT");
						send(cs, strMsg, strlen(strMsg) + 1, 0);
						ZeroMemory(strMsg, 4096);
						for (int k = 0; k < numKeys; k++)
						{
							GenerateKey(key);
							/*for (int i = 0; i < countAllKeys; i++)
							{
								isAlike = 0;
								for (int j = 0; j < KEY_SIZE_FULL; j++)
								{
										if (allKeys[i][j] == key[j]) 
										{
											isAlike++;
										}
										else
										{
											break;
										}
								}
								if (isAlike == KEY_SIZE_FULL)
								{
									printf("Found repeated key:\n");
									k--;
									break;
								}
							}

							if (isAlike != KEY_SIZE_FULL) {
								for (int w = 0; w < KEY_SIZE_FULL; w++)
								{
									allKeys[countAllKeys][w] = key[w];
								}*/
							keyString = StringFromKey(key); 

							if (KeyInFile(keyString))
							{
								printf("Encontrada chave repetida.\n");
								k--;
								break;
							}

							SaveStringToFile(keyString);
							strcat_s(strMsg, 4096, keyString);
							strcat_s(strMsg, 4096, "\r\n");
							countAllKeys++;
							//}
						}
						strcat_s(strMsg, 4096, "Generated ");
						sprintf_s(&strMsg[strlen(strMsg)], sizeof(int), "%d", countAllKeys);
						strcat_s(strMsg, 4096, " keys so far\n");
						time_t now = time(0);
						char* dt = ctime(&now);
						strcat_s(strMsg, 4096, "Sent at: ");
						strcat_s(strMsg, 4096, dt);
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
			send(cs, strMsg, strlen(strMsg) + 1, 0);
			printf("Response: %s\n", strMsg);
		}
	}
}

/// <summary>
/// Função que gera chaves do Euromilhões e guarda no array passado por parâmetro.
/// Output: XX XX XX XX XX EE EE
/// </summary>
/// <param name="key">- array onde será guardada a chave</param>
void GenerateKey(int* key) {
	// Gerar chave - números aleatórios 1-50 - e guardar no array.  
	for (int i = 0; i < KEY_SIZE_NO_STARS; i++)
	{
		key[i] = rand() % 50 + 1;
		for (int j = 0; j < i; j++) // Verificar números repetidos
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

	// Gerar estrelas - números 1-12
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

// Função de comparação a utilizar no qsort()
int cmp_fnc(const void* a, const void* b) {
	return (*(int*)a - *(int*)b);
}

/// <summary>
/// Retorna uma string com a chave passada à função.
/// </summary>
/// <param name="key">Array que contém a chave ordenada</param>
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
/// <param name="message">- string com a(s) chave(s) a guardar</param>
void SaveStringToFile(char message[])
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
	fclose(fp);
}

char* readLine(FILE* infile)
{
	int n = 0, size = 128, ch;
	char* line;
	line = malloc(size + 1);
	while ((ch = getc(infile)) != '\n' && ch != EOF) {
		if (n == size) {
			size = 2;
			line = realloc(line, size + 1);
		}
		line[n++] = ch;
	}
	if (n == 0 && ch == EOF) {
		free(line);
		return NULL;
	}
	line[n] = '\0';
	line = realloc(line, n + 1);
	return line;
}

int KeyExistsInFile(char message[]) {
	char key = NULL;
	int exists = 0;

	FILE* fp = fopen("keys.txt", "r");
	while (key != EOF)
	{
		key = readLine(fp);
		if (strcmp(key, message) == 0) {
			exists = 1;
		}
	}
	fclose(fp);

	return exists;
}

int KeyInFile(char message[]) {
	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	int exists = 0;

	fp = fopen("key.txt", "r");
	if (fp == NULL)
	{
		printf("\nErro a abrir ficheiro.\n");
	}

	while ((fgets(&line, sizeof(message), fp)) != -1)
	{
		if (strcmp(line ,message) == 0)
		{
			exists = 1;
		}
	}
	fclose(fp);
	return exists;
}