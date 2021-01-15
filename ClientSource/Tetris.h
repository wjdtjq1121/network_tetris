#include <WinSock2.h>

#ifndef __TETRIS_H__
#define __TETRIS_H__

#define LEFT	75
#define RIGHT	77
#define UP		72
#define DOWN	80
#define ESC		27

#define FALSE	0
#define TRUE	1

#define ACTIVE_BLOCK	-2
#define CEILLING		-1
#define EMPTY			0
#define WALL			1
#define INACTIVE_BLOCK	2

#define MAIN_X	12
#define MAIN_Y	23

#define MAIN_X_ADJ	5
#define MAIN_Y_ADJ	3
#define ENEMY_X_ADJ	30
#define ENEMY_Y_ADJ	3

#define HEADER_OUT	0
#define WIN			1
#define LOSE		2


// Functions
void sendData(SOCKET hSocket);

void reset(void);
void reset_main(void);

void draw_main(int startX, int startY);
void draw_enemy_main(int startX, int startY);
void new_block(void);
int check_key(void);
void drop_block(void);
int check_crush(int bx, int by, int rotation);
void move_block(int dir);
void check_line(void);

int check_game_over(void);

DWORD WINAPI tetrisMain(LPVOID hSocket);
DWORD WINAPI enemyTetrisMain(LPVOID hSocket);

// Variables
extern const int arrSize;

extern int b_type;
extern int b_rotation;
extern int b_type_next;

extern int main_org[MAIN_Y][MAIN_X];
extern int main_cpy[MAIN_Y][MAIN_X];
extern int enemy_org[MAIN_Y][MAIN_X];
extern int enemy_cpy[MAIN_Y][MAIN_X];

extern int bx, by;
extern int key;
extern int speed;
extern int i, j, k, l;
extern int block_amount;

extern int new_block_on;
extern int crush_on;

extern int blocks[7][4][4][4];

#endif