#include "stdafx.h"


#include <stdio.h>
#include <fcntl.h>

#include <io.h>











static const WORD MAX_CONSOLE_LINES = 500;



#ifdef _DEBUG


void RedirectIOToConsole()

{

int hConHandle;

long lStdHandle;

CONSOLE_SCREEN_BUFFER_INFO coninfo;

FILE *fp;



AllocConsole();



GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);

coninfo.dwSize.Y = MAX_CONSOLE_LINES;

SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);



lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);

hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

fp = _fdopen( hConHandle, "w" );

*stdout = *fp;

setvbuf( stdout, NULL, _IONBF, 0 );



lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);

hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

fp = _fdopen( hConHandle, "r" );

*stdin = *fp;

setvbuf( stdin, NULL, _IONBF, 0 );



lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);

hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

fp = _fdopen( hConHandle, "w" );

*stderr = *fp;

setvbuf( stderr, NULL, _IONBF, 0 );







}

#endif



