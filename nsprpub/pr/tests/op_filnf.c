



















































#include "prinit.h"
#include "prmem.h"
#include "prio.h"
#include "prerror.h"
#include <stdio.h>
#include "plgetopt.h"

#ifdef XP_MAC
#include "prlog.h"
#define printf PR_LogPrint
#else
#endif

static PRFileDesc *t1;
PRIntn error_code;

int main(int argc, char **argv)
{


#ifdef XP_MAC
	SetupMacPrintfLog("pr_open_re.log");
#endif
	

    PR_STDIO_INIT();
	t1 = PR_Open("/usr/tmp/ttools/err03.tmp", PR_TRUNCATE | PR_RDWR, 0666);
	if (t1 == NULL) {
		if (PR_GetError() == PR_FILE_NOT_FOUND_ERROR) {
				printf ("error code is %d \n", PR_GetError());
				printf ("PASS\n");
				return 0;
		}
		else {
				printf ("error code is %d \n", PR_GetError());
				printf ("FAIL\n");
			return 1;
		}
	}
	PR_Close(t1);
	printf ("opened a file that should not exist\n");
	printf ("FAIL\n");
	return 1;
}			
