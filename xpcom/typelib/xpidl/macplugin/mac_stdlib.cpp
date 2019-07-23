











































#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>



jmp_buf exit_jump;
int exit_status = 0;

void std::exit(int status)
{
	exit_status = status;
	longjmp(exit_jump, -1);
}
