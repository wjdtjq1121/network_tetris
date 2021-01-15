#ifndef __CONSOLE_H__
#define __CONSOLE_H__

typedef enum { NOCURSOR, SOLIDCURSOR, NORMALCURSOR } CURSOR_TYPE; //커서숨기는 함수에 사용되는 열거형

void gotoxy(int x, int y);
void setcursortype(CURSOR_TYPE c);

#endif