#pragma once

#include <WinSock2.h>

#define BUF_SIZE	2048
#define MAX_CLNT	256

#define CHESS		1
#define TETRIS		2

#define READ		3
#define WRITE		5
#define SELECT_ROOM	1
#define PLAYING		3

#define TRUE		1
#define FALSE		0

extern const int MAX_MEMBER;
extern const int HEADER_OUT;
extern const int WIN;
extern const int LOSE;

typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;
	int roomNum;
	int state;
	/*
		1. Select room
		2. Wait for other player
		3. Playing game
	*/
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;
} PER_IO_DATA, *LPPER_IO_DATA;

typedef struct _ROOMSTATUS
{
	int status;
	int clntNum;
	int ioCheck;
	int roomNum;
	SOCKET roomHeader;
	SOCKET *roomMember;
	struct _ROOMSTATUS *next;
} ROOMSTATUS, *LPROOMSTATUS;

typedef struct _LIST
{
	int roomCnt;
	LPROOMSTATUS head;
} LinkedList, *pLinkedList;

// About List
pLinkedList makeList();
int deleteList(pLinkedList pList);

LPROOMSTATUS makeRoom(pLinkedList pList, SOCKET roomHeader);
int deleteRoom(pLinkedList pList, LPROOMSTATUS dRoom);
LPROOMSTATUS findRoom(LinkedList *p, int roomNum);

DWORD WINAPI TetrisThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char *message);