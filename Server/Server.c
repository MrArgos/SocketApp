
/*  Trabalho realizado por João Costa al59259, Luís Ribeiro al68708 e Pedro Monteiro al69605  */

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
int KeyExistsInfile(char message[]);
int CountKeysInFile();
DWORD WINAPI KeyGenerator(LPVOID lpParam);
HANDLE fileMutex;

int main()
{
	// Inicializar Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &wsData);
	if (wsResult != 0) {
		fprintf(stderr, "\nWinsock setup fail! Error Code : %d\n", WSAGetLastError());
		return 1;
	}

	// Criar o Mutex para controlar o acesso ao ficheiro das chaves
	fileMutex = CreateMutex(NULL, FALSE, NULL);
	if (fileMutex == NULL)
	{
		printf("Erro na criacao do Mutex: %d\n", GetLastError());
		return 1;
	}

	// Criar socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		fprintf(stderr, "\nSocket creationg fail! Error Code : %d\n", WSAGetLastError());
		return 1;
	}

	printf("\nA espera de conexao...\n");

	// Bind ao socket (endereço IP e porta)
	struct sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(DS_TEST_PORT);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (struct sockaddr*)&hint, sizeof(hint));

	// por o socket em modo listening
	listen(listening, SOMAXCONN);

	struct sockaddr_in client;
	int clientSize;
	SOCKET clientSocket;
	SOCKET* ptclientSocket;
	DWORD dwThreadId;
	HANDLE hThread;
	int conresult = 0;

	// Criação das threads para atendimento de multiplos clientes
	while (TRUE)
	{	
		// criação dos sockets para cada cliente
		clientSize = sizeof(client);
		clientSocket = accept(listening, (struct sockaddr*)&client, &clientSize);
		ptclientSocket = &clientSocket;

		printf("\nNova conexao com endereco: %s\n", inet_ntoa(client.sin_addr));

		// criação da thread respetiva a cada cliente
		hThread = CreateThread(NULL, 0, KeyGenerator, ptclientSocket, 0, &dwThreadId);

		if (hThread == NULL)
		{
			printf("\nErro de criacao de Thread.\n");
		}
	}

	closesocket(clientSocket);
	closesocket(listening);
	CloseHandle(fileMutex);
	WSACleanup();
}

DWORD WINAPI KeyGenerator(LPVOID lpParam)
{
	char strMsg[4096];
	char strRec[4096];

	SOCKET cs;
	SOCKET* ptCs;
	ptCs = (SOCKET*)lpParam;
	cs = *ptCs;
	DWORD dwWaitResult;

	// variáveis para geração de chaves
	int key[KEY_SIZE_FULL];
	char message[2000] = "";
	const char* keyString;
	char* partialMsg;
	int countAllKeys = 0;
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
			printf("\nErro na rececao!\n");
			break;
		}
		if (bytesReceived == 0) {
			printf("\nCliente desligado!\n");
			break;
		}

		if (strcmp(strRec, "\r\n") != 0)
		{

			printf("%i : %s\n", ++msgCount, strRec);

			if (strcmp(strRec, "QUIT") == 0) {
				strcpy(strMsg, "400 BYE");
				send(cs, strMsg, strlen(strMsg) + 1, 0);
				printf("Response:%s\n", strMsg);
				break;
			}

			partialMsg = strtok(strRec, " ");
			// Verifica se recebeu o comando GETKEY
			if (strcmp(partialMsg, "GETKEY") == 0)
			{
				// verifica o primeiro parametro do comando GETKEY
				partialMsg = strtok(NULL, " ");
				if (partialMsg != NULL) {
					numKeys = atoi(partialMsg);
					// Se o parametro for um numero, gera esse numero de chaves
					if (numKeys) {
						strcpy(strMsg, "200 SENDING");
						send(cs, strMsg, strlen(strMsg) + 1, 0);
						printf("Response:\n%s\n", strMsg);
						ZeroMemory(strMsg, 4096);

						// Esperar que o mutex seja libertado
						dwWaitResult = WaitForSingleObject(fileMutex, INFINITE);
						switch (dwWaitResult)
						{
						case WAIT_OBJECT_0:		

							// Quando esta thread tiver o mutex, começa a gerar as chaves
							__try {

								// Gerar numKeys(numero de chaves pedidas) chaves
								for (int k = 0; k < numKeys; k++)
								{
									GenerateKey(key);
									keyString = StringFromKey(key);

									// Verifica se a chave gerada já existe.
									// se existir, decrementa k, gerando mais uma chave
									if (KeyExistsInfile(keyString))
									{
										printf("\nEncontrada chave repetida.\n");
										k--;		// Gerar mais uma chave
										break;
									}

									// Guardar a chave no ficheiro "chaves.txt"
									SaveStringToFile(keyString);
									strcat_s(strMsg, 4096, keyString);
								}
								// conta numero de chaves geradas, e adiciona à mensagem a enviar
								countAllKeys = CountKeysInFile();
								strcat_s(strMsg, 4096, "This server has generated ");
								sprintf_s(&strMsg[strlen(strMsg)], sizeof(int) * 64, "%d", countAllKeys);
								strcat_s(strMsg, 4096, " keys so far.\n");
								time_t now = time(0);
								char* dt = ctime(&now);
								strcat_s(strMsg, 4096, "The current date and time is: ");
								strcat_s(strMsg, 4096, dt);

							}

							__finally {		// Libertar o Mutex quando terminar a geração das chaves
								if (!ReleaseMutex(fileMutex))
								{
									printf("\nErro a libertar Mutex\n");
								}
							}
							break;
						case WAIT_ABANDONED:
							printf("\nGeracao de chaves falhour por espera do Mutex.\n");
							break;
						}
						
					}
					else	// Caso o paramentro do comando GETKEY nao seja um esperado
					{
						strcpy(strMsg, "302 UNEXPECTED");
					}
				}
				else	// Caso o paramentro do comando GETKEY nao esteja presente
				{
					strcpy(strMsg, "301 MISSING");
				}
			}
			else	// Caso nao seja reconhecido o comando
			{
				strcpy(strMsg, "300 UNRECOGNISED");
			}
			send(cs, strMsg, strlen(strMsg) + 1, 0);
			printf("Response:\n%s\n", strMsg);
		}
	}
	return 0;
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
/// Transforma array de int em string contendo a chave
/// </summary>
/// <param name="key">- Array que contém a chave ordenada</param>
/// <returns>char* str - string com a chave</returns>
const char* StringFromKey(int* key) {
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
	strcat_s(str, 2000, "\n");
	return str;
}

/// <summary>
/// Acrescenta a(s) chave(s) a um ficheiro.
/// </summary>
/// <param name="message">- string com a(s) chave(s) a guardar</param>
void SaveStringToFile(char message[])
{
	FILE* fp;
	fp = fopen("chaves.txt", "a");
	if (fp == NULL)
	{
		fprintf(stderr, "\nErro a abrir ficheiro.\n");
		return;
	}
	if (fputs(message, fp) < 0)
	{
		fprintf(stderr, "\nErro a escrever no ficheiro.\n");
		return;
	}
	fclose(fp);
}

/// <summary>
/// Verifica se uma chave já se encontra no ficheiro das chaves
/// </summary>
/// <param name="message">- string com a chave a verificar</param>
/// <returns> Retorna 1 se chave for repetida, 0 senão se encontrar no ficheiro</returns>
int KeyExistsInfile(char message[]) {
	FILE* fp;
	char keyRead[50];
	int keyExists = 0;
	
	fp = fopen("chaves.txt", "r");
	if (fp == NULL)
	{
		printf("\nErro a ler ficheiro\n");
		return 0;
	}

	while (fgets(keyRead, 50, fp) != NULL)
	{
		if (strcmp(keyRead, message) == 0) {
			keyExists = 1;
			return keyExists;
		}
	}
	return keyExists;
}

/// <summary>
/// Conta o numero de chaves no ficheiro das chaves
/// </summary>
/// <returns> int com numero de chaves</returns>
int CountKeysInFile() {
	FILE* fp;
	char keyRead[50];
	int keyCount = 0;

	fp = fopen("chaves.txt", "r");
	if (fp == NULL)
	{
		printf("\nErro a ler ficheiro\n");
		return 0;
	}

	while (fgets(keyRead, 50, fp) != NULL)
	{
		keyCount++;
	}
	return keyCount;
}
