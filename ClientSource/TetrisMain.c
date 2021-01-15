#include <stdio.h>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <math.h>
#include "GameClient.h"
#include "Tetris.h"
#include "Console.h"

DWORD WINAPI tetrisMain(LPVOID hSocket)
{
	SOCKET hSock = *((SOCKET *)hSocket);
	int i, status;

	srand((unsigned int)time(NULL) + getpid());
	setcursortype(NOCURSOR);
	reset();
	
	while (1) {
		for (i = 0; i < 5; i++) {
			status = check_key();
			if (status == FALSE)
				return HEADER_OUT;
			EnterCriticalSection(&cs);
			draw_main(MAIN_X_ADJ, MAIN_Y_ADJ);
			LeaveCriticalSection(&cs);
			Sleep(100);
			if (crush_on && check_crush(bx, by + 1, b_rotation) == FALSE) Sleep(100);
		}
		sendData(hSock);
		drop_block();

		status = check_game_over();
		if (status == TRUE) {
			EnterCriticalSection(&cs);
			gotoxy(MAIN_X_ADJ, MAIN_Y_ADJ + MAIN_Y + 2);
			printf("You are loser");
			LeaveCriticalSection(&cs);
			return LOSE;
		}
		if (new_block_on == 1) new_block();
	}
}

DWORD WINAPI enemyTetrisMain(LPVOID hSocket)
{
	SOCKET hSock = *((SOCKET *)hSocket);
	while (1) {
		int recvByte = recv(hSock, &enemy_org, sizeof(int) * MAIN_X * MAIN_Y, 0);
		if (recvByte > 4) {
			EnterCriticalSection(&cs);
			draw_enemy_main(ENEMY_X_ADJ, ENEMY_Y_ADJ);
			LeaveCriticalSection(&cs);
		}
		else {
			int code = enemy_org[0][0];
			switch (code) {
			case HEADER_OUT:
				EnterCriticalSection(&cs);
				gotoxy(ENEMY_X_ADJ, ENEMY_Y_ADJ + MAIN_Y + 2);
				printf("Enemy out");
				LeaveCriticalSection(&cs);
				return 0;
			case WIN:
				EnterCriticalSection(&cs);
				gotoxy(ENEMY_X_ADJ, ENEMY_Y_ADJ + MAIN_Y + 2);
				printf("You are winner");
				LeaveCriticalSection(&cs);
				return 0;
			}
		}
	}
}