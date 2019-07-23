


















































#include "prinit.h"
#include "prmem.h"
#include "prio.h"
#include "prerror.h"
#include <stdio.h>
#include "plgetopt.h"




#define NO_SUCH_FILE "/no/such/file.tmp"

static PRFileDesc *t1;

int main(int argc, char **argv)
{
    PR_STDIO_INIT();
	t1 = PR_Open(NO_SUCH_FILE,  PR_RDONLY, 0666);
	if (t1 == NULL) {
		if (PR_GetError() == PR_FILE_NOT_FOUND_ERROR) {
			printf ("error code is PR_FILE_NOT_FOUND_ERROR, as expected\n");
			printf ("PASS\n");
			return 0;
		} else {
			printf ("error code is %d \n", PR_GetError());
			printf ("FAIL\n");
			return 1;
		}
	}
	printf ("File %s exists on this machine!?\n", NO_SUCH_FILE);
	if (PR_Close(t1) == PR_FAILURE) {
		printf ("cannot close file\n");
		printf ("error code is %d \n", PR_GetError());
	}
	printf ("FAIL\n");
	return 1;
}			
