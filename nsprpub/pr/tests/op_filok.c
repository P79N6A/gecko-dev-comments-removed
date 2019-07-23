


















































#include "prinit.h"
#include "prmem.h"
#include "prio.h"
#include "prerror.h"
#include <stdio.h>

#ifdef XP_MAC
#include "prlog.h"
#define printf PR_LogPrint
#else
#endif





#ifdef VMS
#define EXISTING_FILENAME "SYS$LOGIN:LOGIN.COM"
#elif defined(SYMBIAN)
#define EXISTING_FILENAME "z:\\system\\install\\Series60v3.0.sis"
#elif defined (XP_UNIX)
#define EXISTING_FILENAME "/bin/sh"
#elif defined(WIN32)
#define EXISTING_FILENAME "c:/autoexec.bat"
#elif defined(OS2)
#define EXISTING_FILENAME "c:/config.sys"
#elif defined(BEOS)
#define EXISTING_FILENAME "/boot/beos/bin/sh"
#else
#error "Unknown OS"
#endif

static PRFileDesc *t1;

int main(int argc, char **argv)
{

#ifdef XP_MAC
	SetupMacPrintfLog("pr_open_re.log");
#endif
	
    PR_STDIO_INIT();

	t1 = PR_Open(EXISTING_FILENAME, PR_RDONLY, 0666);

	if (t1 == NULL) {
		printf ("error code is %d \n", PR_GetError());
		printf ("File %s should be found\n",
				EXISTING_FILENAME);
		return 1;
	} else {
		if (PR_Close(t1) == PR_SUCCESS) {
			printf ("Test passed \n");
			return 0;
		} else {
			printf ("cannot close file\n");
			printf ("error code is %d\n", PR_GetError());
			return 1;
		}
	}
}			
