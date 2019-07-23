


















































#include "prinit.h"
#include "prmem.h"
#include "prio.h"
#include "prerror.h"
#include <stdio.h>
#include "plerror.h"
#include "plgetopt.h"

static PRFileDesc *t1;
PRIntn error_code;






#define TOO_LONG 5000

int main(int argc, char **argv)
{
	char nameTooLong[TOO_LONG];
	int i;

	
	for (i = 0; i < TOO_LONG - 1; i++) {
		if (i % 10 == 0) {
			nameTooLong[i] = '/';
		} else {
			nameTooLong[i] = 'a';
		}
	}
	nameTooLong[TOO_LONG - 1] = 0;

    PR_STDIO_INIT();
	t1 = PR_Open(nameTooLong, PR_RDWR, 0666);
	if (t1 == NULL) {
		if (PR_GetError() == PR_NAME_TOO_LONG_ERROR) {
            PL_PrintError("error code is");
			printf ("PASS\n");
			return 0;
		}
		else {
            PL_PrintError("error code is");
			printf ("FAIL\n");
			return 1;
		}
	}
	
		else {
			printf ("Test passed\n");
			return 0;
		}
	


}			
