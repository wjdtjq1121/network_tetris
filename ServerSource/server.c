#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <WinSock2.h>
#include <time.h>

#include "GameServer.h"

CRITICAL_SECTION cs;
pLinkedList tetrisRoomList;

void printList(pLinkedList p) {
	LPROOMSTATUS pr = p->head;
	while (pr != NULL) {
		printf("%d Room : %d, %d, %d\n", pr->roomNum, pr->clntNum, pr->roomHeader, pr->roomMember[0]);
		pr = pr->next;
	}
}

void setNetworking(HANDLE *hTetrisComPort, SOCKET *hSock) {
	//////////////////////////////////////////////////////////////////
	/////////////////////	Start Networking	//////////////////////
	//////////////////////////////////////////////////////////////////

	WSADATA wsaData;
	SYSTEM_INFO sysInfo;	// for get num of process
	SOCKADDR_IN servAddr;

	int port, i;
	printf("Enter the port: ");
	scanf("%d", &port);	fflush(stdin);

	/*     Network Setting      */
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	*hTetrisComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);	// Set tetris IOCP
	GetSystemInfo(&sysInfo);	// Get system info
	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++)
		_beginthreadex(NULL, 0, TetrisThreadMain, (LPVOID)(*hTetrisComPort), 0, NULL);	// Start thread (number of process)

	*hSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);	// Set server socket to overlapped
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(port);

	if (bind((*hSock), (SOCKADDR *)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)	// Bind
		ErrorHandling("bind() error!");

	if (listen((*hSock), 5) == SOCKET_ERROR)	// Listen
		ErrorHandling("listen() error!");
}

void sendRoomList(SOCKET sock) {
	if (sock == 0) return;

	// For select room
	EnterCriticalSection(&cs);
	send(sock, &(tetrisRoomList->roomCnt), sizeof(int), 0);	// Send how many rooms in server
	send(sock, &MAX_MEMBER, sizeof(int), 0);	// Send how many rooms in server
	LPROOMSTATUS room = tetrisRoomList->head;
	for (int i = 0; i < tetrisRoomList->roomCnt; i++) {				// Send room member
		send(sock, &room->roomNum, sizeof(int), 0);
		send(sock, (char *)&(room->clntNum), sizeof(int), 0);
		send(sock, &room->status, sizeof(int), 0);
		room = room->next;
	}
	LeaveCriticalSection(&cs);
}

int main(int argc, char *argv[])
{
	HANDLE hTetrisComPort;	// IOCP handle
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	int recvBytes, flags = 0;

	setNetworking(&hTetrisComPort, &hServSock);
	InitializeCriticalSection(&cs);	// Create critical section
	tetrisRoomList = makeList();

	// Do Something
	while (1) {
		SOCKET hClntSock;
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);

		hClntSock = accept(hServSock, (SOCKADDR *)&clntAddr, &addrLen);	// Accept to hClntSock

		printf("connected %s %d\n", inet_ntoa(clntAddr.sin_addr), hClntSock);	// Loging

		// Make handleInfo for IOCP
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		handleInfo->roomNum = -1;
		handleInfo->state = SELECT_ROOM;
		memcpy(&(handleInfo->clntAddr), &clntAddr, addrLen);

		// Connect to hClntSock, IOCP, handleInfo
		sendRoomList(hClntSock);
		CreateIoCompletionPort((HANDLE)hClntSock, hTetrisComPort, (DWORD)handleInfo, 0);
		
		// Make ioInfo for GetQueuedCompletionStatus()
		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;
		
		// Set notify for receive
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
	}
	DeleteCriticalSection(&cs);
	WSACleanup();
	return 0;
}

DWORD WINAPI TetrisThreadMain(LPVOID CompletionPortIO)
{
	SOCKET sock;	// Client socket handle
	HANDLE hComPort = (HANDLE)CompletionPortIO;	// IOCP handle
	DWORD bytesTrans;	// Receive byte
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;
	LPROOMSTATUS pRoom = NULL;
	int i, roomNum, status;

	/////////////////////////////////////////////////////////////
	/////////////////////// Start thread ////////////////////////
	/////////////////////////////////////////////////////////////

	while (1) {
		// Wait for notify
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);

		sock = handleInfo->hClntSock;	// Client handle
		roomNum = handleInfo->roomNum;	// Client roomNumber

		if (ioInfo->rwMode == READ) {
			switch (handleInfo->state) {
			case SELECT_ROOM:
				if (bytesTrans == 0) {
					closesocket(sock);
					free(handleInfo);
					break;
				}

				status = FALSE;
				roomNum = (int)*(ioInfo->wsaBuf.buf);
				if (roomNum == 0) {	// Make room
					EnterCriticalSection(&cs);
					pRoom = makeRoom(tetrisRoomList, sock);	// Make room
					LeaveCriticalSection(&cs);

					handleInfo->roomNum = pRoom->roomNum;
					handleInfo->state = PLAYING;
					send(sock, &(handleInfo->roomNum), 4, 0);
					status = TRUE;
				}
				else {	// Enter room
					EnterCriticalSection(&cs);
					pRoom = findRoom(tetrisRoomList, roomNum);
					LeaveCriticalSection(&cs);
					
					if (pRoom == NULL)
						status = FALSE;
					else if (pRoom->clntNum < MAX_MEMBER + 1 && pRoom->status == TRUE) {
						handleInfo->roomNum = roomNum;
						EnterCriticalSection(&cs);
						for (i = 0; i < MAX_MEMBER; i++) {
							if (pRoom->roomMember[i] == NULL) {
								pRoom->roomMember[i] = sock;
								break;
							}
						}
						(pRoom->clntNum)++;
						LeaveCriticalSection(&cs);
						handleInfo->state = PLAYING;
						status = TRUE;
					}
					else
						status = FALSE;
				}
				send(sock, &status, sizeof(int), 0);

				if (status == TRUE) {
					// Send "start game"
					if (pRoom->clntNum == MAX_MEMBER + 1) {
						send(pRoom->roomHeader, &status, sizeof(int), 0);
						for (i = 0; i < MAX_MEMBER; i++) {
							if (pRoom->roomMember[i] != NULL)
								send(pRoom->roomMember[i], &status, sizeof(int), 0);
						}
						EnterCriticalSection(&cs);
						pRoom->status = FALSE;
						LeaveCriticalSection(&cs);
					}
				}
				else
					sendRoomList(sock);
				break;
			case PLAYING:

				EnterCriticalSection(&cs);
				pRoom = findRoom(tetrisRoomList, roomNum);
				LeaveCriticalSection(&cs);

				{
					if (pRoom == NULL) {
						sendRoomList(sock);
						break;
					}
					if (pRoom->roomHeader != sock) {
						for (i = 0; i < MAX_MEMBER; i++) {
							if (pRoom->roomMember[i] == sock) break;
						}
						if (i == MAX_MEMBER) break;
					}
				}

				if (bytesTrans == 0) {
					if (pRoom->roomHeader == sock) {
						printf("%d room header out\n", roomNum);
						int st = deleteRoom(tetrisRoomList, pRoom);
						if (st == FALSE) {	// If there is another member
							for (i = 0; i < MAX_MEMBER; i++) {
								if (pRoom->roomMember[i] != NULL) {
									send(pRoom->roomMember[i], (int *)&HEADER_OUT, sizeof(int), 0);
									pRoom->roomMember[i] = NULL;
									(pRoom->clntNum)--;
								}
							}
							deleteRoom(tetrisRoomList, pRoom);
						}
					}
					else {
						for (i = 0; i < MAX_MEMBER; i++) {
							if (pRoom->roomMember[i] == sock) {
								printf("%d room Member out %d\n", roomNum, sock);
								pRoom->roomMember[i] = NULL;
								(pRoom->clntNum)--;
								break;
							}
						}
					}
					closesocket(sock);
					free(handleInfo);
					continue;
				}

				else if (bytesTrans <= 4) {	// Something happend (win / lose / exit)
					int code = (int)*(ioInfo->wsaBuf.buf);
					switch (code) {
					case 0:	// exit
						EnterCriticalSection(&cs);
						if (pRoom->roomHeader != sock) {
							send(pRoom->roomHeader, (int *)&HEADER_OUT, sizeof(int), 0);
							for (i = 0; i < MAX_MEMBER; i++) {
								if (pRoom->roomMember[i] != NULL) {
									if (pRoom->roomMember[i] != sock)
										send(pRoom->roomMember[i], (int *)&HEADER_OUT, sizeof(int), 0);
									else {
										pRoom->roomMember[i] = NULL;
										(pRoom->clntNum)--;
									}
								}
							}
						}
						else {
							int st = deleteRoom(tetrisRoomList, pRoom);
							if (st == FALSE) {	// If there is another member
								for (i = 0; i < MAX_MEMBER; i++) {
									if (pRoom->roomMember[i] != NULL) {
										send(pRoom->roomMember[i], (int *)&HEADER_OUT, sizeof(int), 0);
										pRoom->roomMember[i] = NULL;
										(pRoom->clntNum)--;
									}
								}
								deleteRoom(tetrisRoomList, pRoom);
							}
						}
						LeaveCriticalSection(&cs);
						break;
					case 2:	// LOSE
						EnterCriticalSection(&cs);
						if (pRoom->roomHeader == sock) {
							for (i = 0; i < MAX_MEMBER; i++) {
								if (pRoom->roomMember[i] != NULL) {
									send(pRoom->roomMember[i], (int *)&WIN, sizeof(int), 0);
									pRoom->roomMember[i] = NULL;
									pRoom->clntNum--;
								}
							}
							deleteRoom(tetrisRoomList, pRoom);
						}
						else {
							for (i = 0; i < MAX_MEMBER; i++) {
								if (pRoom->roomMember[i] != NULL) {
									if (pRoom->roomMember[i] != sock) {
										send(pRoom->roomMember[i], (int *)&WIN, sizeof(int), 0);
										pRoom->roomMember[i] = NULL;
										pRoom->clntNum--;
									}
								}
							}
							send(pRoom->roomHeader, (int *)&WIN, sizeof(int), 0);
						}
						LeaveCriticalSection(&cs);
						break;
					}
					handleInfo->state = SELECT_ROOM;
					sendRoomList(sock);
				}
				else {
					memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
					ioInfo->wsaBuf.len = bytesTrans;
					ioInfo->rwMode = WRITE;

					// Send received data
					EnterCriticalSection(&cs);
					if (pRoom->roomHeader != sock)
						WSASend(pRoom->roomHeader, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
					for (i = 0; i < MAX_MEMBER; i++) {
						if (pRoom->roomMember[i] == NULL || pRoom->roomMember[i] == sock) continue;
						WSASend(pRoom->roomMember[i], &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
					}
					LeaveCriticalSection(&cs);

					ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
					memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
					ioInfo->wsaBuf.len = BUF_SIZE;
					ioInfo->wsaBuf.buf = ioInfo->buffer;
					ioInfo->rwMode = READ;
					break;
				}
			}

			EnterCriticalSection(&cs);
			printList(tetrisRoomList);
			LeaveCriticalSection(&cs);

			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		else {
			EnterCriticalSection(&cs);
			pRoom = findRoom(tetrisRoomList, roomNum);
			if (pRoom != NULL) {
				// If room's client number is equal with number of data sended
				(pRoom->ioCheck)++;
				if (pRoom->ioCheck == pRoom->clntNum) {
					free(ioInfo);
					pRoom->ioCheck = 0;
				}
			}
			LeaveCriticalSection(&cs);
		}

	}
	return 0;
}