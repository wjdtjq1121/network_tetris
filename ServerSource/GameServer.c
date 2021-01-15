#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <assert.h>
#include "GameServer.h"

const int MAX_MEMBER = 1;
extern const int HEADER_OUT = 0;
extern const int WIN = 1;
extern const int LOSE = 2;

pLinkedList makeList()
{
	pLinkedList newList = (pLinkedList)malloc(sizeof(LinkedList));
	assert(newList != NULL);

	newList->roomCnt = 0;
	newList->head = NULL;
	return newList;
}

int deleteList(pLinkedList pList)
{
	char c;
	if (pList->head != NULL) {
		while (1) {
			fflush(stdin);	// For flush the stdin buffer
			printf("There is non-freed data in list.\nAre you sure that delete the list? (Y/N) ");
			c = getchar();
			if (c == 'Y' || c == 'y') {
				while (pList->head != NULL) {
					LPROOMSTATUS delRoom = pList->head;
					pList->head = pList->head->next;
					free(delRoom);
				}
				break;
			}
			else if (c == 'N' || c == 'n') return FALSE;
			else printf("Enter correctly.\n");
		}
	}
	free(pList);
	return TRUE;
}

LPROOMSTATUS makeRoom(pLinkedList pList, SOCKET roomHeader) {
	LPROOMSTATUS newRoom = (LPROOMSTATUS)malloc(sizeof(ROOMSTATUS));
	assert(newRoom != NULL);

	newRoom->status = TRUE;
	newRoom->clntNum = 1;
	newRoom->ioCheck = 0;
	newRoom->roomHeader = roomHeader;
	newRoom->roomMember = (SOCKET *)calloc(MAX_MEMBER, sizeof(SOCKET));
	
	newRoom->next = NULL;
	if (pList->head == NULL) {
		pList->head = newRoom;
		newRoom->roomNum = 1;
	}
	else {
		LPROOMSTATUS temp = pList->head;
		while (temp->next != NULL)
			temp = temp->next;
		temp->next = newRoom;
		newRoom->roomNum = temp->roomNum + 1;
	}

	(pList->roomCnt)++;

	return newRoom;
}

int deleteRoom(pLinkedList pList, LPROOMSTATUS pRoom) {
	LPROOMSTATUS delRoom = pList->head;
	LPROOMSTATUS prevRoom = delRoom;

	if (pList == NULL) return FALSE;
	else if (pList->head == NULL) {
		printf("There is no room\n");
		return FALSE;
	}
	else if (pRoom->clntNum > 1) {
		fputs("Can't delete room. There is a remainder.\n", stderr);
		return FALSE;
	}

	while (delRoom != NULL) {
		if (delRoom == pRoom) {
			break;
		}

		prevRoom = delRoom;
		delRoom = delRoom->next;
	}

	if (delRoom == prevRoom)
		pList->head = delRoom->next;
	else
		prevRoom->next = delRoom->next;

	free(delRoom->roomMember);
	free(delRoom);
	(pList->roomCnt)--;
	printf("Room deleted\n");
	return TRUE;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

LPROOMSTATUS findRoom(LinkedList *p, int roomNum)
{
	LPROOMSTATUS pRoom = p->head;
	for (int i = 0; i < p->roomCnt; i++) {
		if (pRoom != NULL) {
			if (pRoom->roomNum == roomNum) break;
			pRoom = pRoom->next;
		}
	}
	return pRoom;
}