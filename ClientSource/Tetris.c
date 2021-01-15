#include <stdio.h>
#include <WinSock2.h>
#include <windows.h>
#include <conio.h>
#include <stdlib.h>
#include "Tetris.h"
#include "GameClient.h"

const int arrSize = MAIN_X * MAIN_Y;

int b_type;
int b_rotation;
int b_type_next;

int main_org[MAIN_Y][MAIN_X];
int main_cpy[MAIN_Y][MAIN_X];
int enemy_org[MAIN_Y][MAIN_X];
int enemy_cpy[MAIN_Y][MAIN_X];

int bx, by;
int key;
int speed;
int block_amount;

int new_block_on = 0;
int crush_on = 0;

int blocks[7][4][4][4] = {
	{
		{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 }
	},
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
		{ 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
		{ 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0 }
	},
	{
		{ 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0 },
		{ 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0 }
	},
	{
		{ 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0 }
	},
	{
		{ 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0 }
	},
	{
		{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0 },
		{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 }
	},
	{
		{ 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0 }
	}
};


void sendData(SOCKET hSocket)
{
	int e;
	WSABUF buf;
	WSAOVERLAPPED overlapped;
	buf.buf = main_org;
	buf.len = sizeof(int) * arrSize;
	memset(&overlapped, 0, sizeof(OVERLAPPED));
	
	if (WSASend(hSocket, &buf, 1, NULL, 0, &overlapped, NULL) == SOCKET_ERROR) {
		if ((e = WSAGetLastError()) != WSA_IO_PENDING) {
			printf("Error Num: %d\n", e);
			ErrorHandling("Data sending Error!");
		}
	}
}

void reset(void)
{
	key = 0;
	crush_on = 0;
	speed = 100;

	system("cls");
	reset_main();
	EnterCriticalSection(&cs);
	draw_main(MAIN_X_ADJ, MAIN_Y_ADJ);
	draw_enemy_main(ENEMY_X_ADJ, ENEMY_Y_ADJ);
	LeaveCriticalSection(&cs);

	b_type_next = rand() % 7;
	new_block();
}

void reset_main(void)
{
	int i, j;

	for (i = 0; i < MAIN_Y; i++) {
		for (j = 0; j < MAIN_X; j++) {
			main_org[i][j] = 0;
			main_cpy[i][j] = 100;
			enemy_org[i][j] = 0;
			enemy_cpy[i][j] = 100;
		}
	}

	for (j = 1; j < MAIN_X; j++) {
		main_org[3][j] = CEILLING;
		enemy_org[3][j] = CEILLING;
	}

	for (i = 1; i < MAIN_Y - 1; i++) {
		main_org[i][0] = WALL;
		enemy_org[i][0] = WALL;
		main_org[i][MAIN_X - 1] = WALL;
		enemy_org[i][MAIN_X - 1] = WALL;
	}

	for (j = 0; j < MAIN_X; j++) {
		main_org[MAIN_Y - 1][j] = WALL;
		enemy_org[MAIN_Y - 1][j] = WALL;
	}
}

void draw_main(int startX, int startY)
{
	int i, j;

	for (j = 1; j < MAIN_X - 1; j++) {
		if (main_org[3][j] == EMPTY)
			main_org[3][j] = CEILLING;
	}

	for (i = 0; i < MAIN_Y; i++) {
		for (j = 0; j < MAIN_X; j++) {
			if (main_cpy[i][j] != main_org[i][j]) {
				gotoxy(startX + j, startY + i);
				switch (main_org[i][j]) {
				case EMPTY:
					printf(" ");
					break;
				case CEILLING:
					printf(". ");
					break;
				case WALL:
					printf("* ");
					break;
				case INACTIVE_BLOCK:
					printf("o");
					break;
				case ACTIVE_BLOCK:
					printf("O");
					break;
				}
			}
		}
	}

	for (i = 0; i < MAIN_Y; i++) {
		for (j = 0; j < MAIN_X; j++) {
			main_cpy[i][j] = main_org[i][j];
		}
	}
}

void draw_enemy_main(int startX, int startY)
{
	int i, j;

	for (j = 1; j < MAIN_X - 1; j++) {
		if (enemy_org[3][j] == EMPTY)
			enemy_org[3][j] = CEILLING;
	}

	for (i = 0; i < MAIN_Y; i++) {
		for (j = 0; j < MAIN_X; j++) {
			if (enemy_cpy[i][j] != enemy_org[i][j]) {
				gotoxy(startX + j, startY + i);
				switch (enemy_org[i][j]) {
				case EMPTY:
					printf(" ");
					break;
				case CEILLING:
					printf(". ");
					break;
				case WALL:
					printf("* ");
					break;
				case INACTIVE_BLOCK:
					printf("o");
					break;
				case ACTIVE_BLOCK:
					printf("O");
					break;
				}
			}
		}
	}

	for (i = 0; i < MAIN_Y; i++) {
		for (j = 0; j < MAIN_X; j++) {
			enemy_cpy[i][j] = enemy_org[i][j];
		}
	}
}

void new_block(void)
{
	int i, j;

	bx = (MAIN_X / 2);
	by = 0;
	b_type = b_type_next;
	b_type_next = rand() % 7;
	b_rotation = 0;

	new_block_on = 0;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			if (blocks[b_type][b_rotation][i][j] == 1)
				main_org[by + i][bx + j] = ACTIVE_BLOCK;
		}
	}
}

int check_key(void)
{
	key = 0;

	if (kbhit()) {
		key = getch();

		if (key == 224) {
			do {
				key = getch();
			} while (key == 224);

			switch (key) {
			case LEFT:
				if (check_crush(bx - 1, by, b_rotation) == TRUE)
					move_block(LEFT);
				break;
			case RIGHT:
				if (check_crush(bx + 1, by, b_rotation) == TRUE)
					move_block(RIGHT);
				break;
			case DOWN:
				if (check_crush(bx, by + 1, b_rotation) == TRUE)
					move_block(DOWN);
				break;
			case UP:
				if (check_crush(bx, by, b_rotation + 1) % 4 == TRUE)
					move_block(UP);
				else if (crush_on == 1 && check_crush(bx, by - 1, (b_rotation + 1) % 4) == TRUE)
					move_block(100);
			}
		}
		else {
			switch (key) {
			case ESC:
				//system("cls");
				EnterCriticalSection(&cs);
				gotoxy(ENEMY_X_ADJ, ENEMY_Y_ADJ + MAIN_Y + 2);
				printf("Go out");
				LeaveCriticalSection(&cs);
				return FALSE;
			}
		}
	}

	while (kbhit())
		getch();

	return TRUE;
}

void drop_block(void)
{
	int i, j;

	if (crush_on && check_crush(bx, by + 1, b_rotation) == TRUE)
		crush_on = 0;
	if (crush_on && check_crush(bx, by + 1, b_rotation) == FALSE) {
		for (i = 0; i < MAIN_Y; i++) {
			for (j = 0; j < MAIN_X; j++) {
				if (main_org[i][j] == ACTIVE_BLOCK)
					main_org[i][j] = INACTIVE_BLOCK;
			}
		}
		crush_on = 0;
		check_line();
		new_block_on = 1;
		return;
	}
	if (check_crush(bx, by + 1, b_rotation) == TRUE)
		move_block(DOWN);
	if (check_crush(bx, by + 1, b_rotation) == FALSE)
		crush_on++;
}

void move_block(int dir)
{
	int i, j;

	switch (dir) {
	case LEFT:
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i][bx + j] = EMPTY;
			}
		}
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i][bx + j - 1] = ACTIVE_BLOCK;
			}
		}
		bx--;
		break;
	case RIGHT:
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i][bx + j] = EMPTY;
			}
		}
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i][bx + j + 1] = ACTIVE_BLOCK;
			}
		}
		bx++;
		break;
	case DOWN:
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i][bx + j] = EMPTY;
			}
		}
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i + 1][bx + j] = ACTIVE_BLOCK;
			}
		}
		by++;
		break;
	case UP:
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i][bx + j] = EMPTY;
			}
		}
		b_rotation = (b_rotation + 1) % 4;
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (blocks[b_type][b_rotation][i][j] == 1)
					main_org[by + i][bx + j] = EMPTY;
			}
		}
		break;
	}
}

void check_line(void)
{
	int i, j, k, l;
	int block_amount;

	for (i = MAIN_Y - 2; i > 3;) {
		block_amount = 0;
		for (j = 1; j < MAIN_X - 1; j++) {
			if (main_org[i][j] > 0)
				block_amount++;
		}
		if (block_amount == MAIN_X - 2) {
			for (k = i; k > 1; k--) {
				for (l = 1; l < MAIN_X - 1; l++) {
					if (main_org[k - 1][l] != CEILLING)
						main_org[k][l] = main_org[k - 1][l];
					if (main_org[k - 1][l] == CEILLING)
						main_org[k][l] = EMPTY;
				}
			}
		}
		else
			i--;
	}
}

int check_game_over(void)
{
	int i;
	int x = 5;
	int y = 5;

	for (i = 1; i < MAIN_X - 2; i++) {
		if (main_org[3][i] > 0) {
/*			gotoxy(x, y + 0); printf("----------------------------------");
			gotoxy(x, y + 1); printf("-                                -");
			gotoxy(x, y + 2); printf("                                 ");
			gotoxy(x, y + 3); printf("        G A M E  O V E R    ");
			gotoxy(x, y + 7); printf("   Press any key to restart..  ");
			while (kbhit()) getch();
			key = getch();
			reset();
*/			return TRUE;
		}
	}
	return FALSE;
}

/*
gotoxy(x, y + 0); printf("----------------------------------");
gotoxy(x, y + 1); printf("-                                -");
gotoxy(x, y + 2); printf("                                 ");
gotoxy(x, y + 3); printf("        G A M E  O V E R    ");
gotoxy(x, y + 7); printf("   Press any key to restart..  ");
while (kbhit()) getch();
key = getch();
reset();
*/

int check_crush(int bx, int by, int b_rotation)
{
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			if (blocks[b_type][b_rotation][i][j] == 1 && main_org[by + i][bx + j] > 0)
				return FALSE;
		}
	}

	return TRUE;
}