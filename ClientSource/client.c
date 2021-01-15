#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <WinSock2.h>

#include "GameClient.h"
#include "Tetris.h"

int is_H = FALSE;
CRITICAL_SECTION cs;

void setNetworking(SOCKET *hSocket) {
	WSADATA wsaData;
	SOCKADDR_IN servAddr;

	char ip[16], port[6];
	printf("Enter the IP and port what you want to connect: ");
	scanf("%s %s", ip, port); fflush(stdin);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	*hSocket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);	// Set socket
	if ((*hSocket) == INVALID_SOCKET)
		ErrorHandling("socket() error!");

	memset(&servAddr, 0, sizeof(SOCKADDR_IN));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(ip);
	servAddr.sin_port = htons(atoi(port));

	if (connect((*hSocket), (SOCKADDR *)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)	// Try connect
		ErrorHandling("connect() error!");
	else
		printf("Connected..........\n");
}

int selectRoom(SOCKET hSocket)
{
	int i, roomNum;
	int roomStatus = TRUE;
	do {
		int roomNM, roomCnt, roomMEM, roomCM, roomST;
		system("CLS");

		if (roomStatus == FALSE)
			printf("Room %d is already full. Reselect!\n", roomNum);

		// Receive room struct status
		recv(hSocket, &roomCnt, sizeof(int), 0);
		recv(hSocket, &roomMEM, sizeof(int), 0);
		if (roomCnt == 0)
			printf("There isn't any room.\n");
		else {
			for (i = 0; i < roomCnt; i++) {
				// Print room status
				recv(hSocket, &roomNM, sizeof(int), 0);
				recv(hSocket, &roomCM, sizeof(int), 0);
				recv(hSocket, &roomST, sizeof(int), 0);
				printf("Room %d. [%d/%d] %c\n", roomNM, roomCM, roomMEM + 1, roomST ? 'Y' : 'F');
			}
		}

		// Select room
		printf("\nSelect or make room (0 to make room):  ");
		scanf("%d", &roomNum); fflush(stdin);	// For flush stdin
												/* roomNum = GetInt() */

		send(hSocket, &roomNum, sizeof(int), 0);	// Send roomNum

		if (roomNum == 0) {							// Make room
			recv(hSocket, &roomNum, sizeof(int), 0);
			is_H = TRUE;
		}

		recv(hSocket, &roomStatus, sizeof(roomStatus), 0);
	} while (roomStatus == FALSE);

	return roomNum;
}

int main(int argc, char *argv[])
{
	SOCKET hSocket;
	HANDLE hMainThread, hEnemyThread;
	int i, roomNum, status = FALSE, enemyState;

	srand((unsigned int)time(NULL) + getpid());

	// Setup Letworking
	setNetworking(&hSocket);
	InitializeCriticalSection(&cs);

	while (1) {
		is_H = FALSE;
		// Select Room
		roomNum = selectRoom(hSocket);

		// Enter that room
		system("CLS");
		printf("Room %d\n", roomNum);

		printf("Waiting for other players...\n");
		recv(hSocket, &status, sizeof(int), 0);

		printf("Game will start after few seconds.\n");
		printf("3 "); Sleep(1000); printf("2 "); Sleep(1000); printf("1 \n"); Sleep(1000);
		printf("Start!"); Sleep(500);

		hMainThread = (HANDLE)_beginthreadex(NULL, 0, tetrisMain, (LPVOID)&hSocket, 0, NULL);
		hEnemyThread = (HANDLE)_beginthreadex(NULL, 0, enemyTetrisMain, (LPVOID)&hSocket, 0, NULL);

		WaitForSingleObject(hMainThread, INFINITE);
		GetExitCodeThread(hEnemyThread, &enemyState);
		GetExitCodeThread(hMainThread, &status);
		if (enemyState == STILL_ACTIVE) {
			TerminateThread(hEnemyThread, 0);
			send(hSocket, &status, sizeof(int), 0);
		}
		else {
			if (is_H == TRUE)
				send(hSocket, &status, sizeof(int), 0);
		}
		getch(); fflush(stdin);
	}
	
	closesocket(hSocket);
	DeleteCriticalSection(&cs);
	WSACleanup();
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	getchar(); getchar();
	exit(1);
}

/*getSize = recv(hSocket, buffer, BUF_SIZE, 0);
buffer[getSize] = 0;
room = (LPROOM)buffer;*/