#ifndef __CONSOLE_H__
#define __CONSOLE_H__

typedef enum { NOCURSOR, SOLIDCURSOR, NORMALCURSOR } CURSOR_TYPE; //Ŀ������� �Լ��� ���Ǵ� ������

void gotoxy(int x, int y);
void setcursortype(CURSOR_TYPE c);

#endif