
























































#include "plgetopt.h"

#include "nspr.h"
#include "prpriv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PRIntn failed_already=0;
PRIntn debug_mode;






#define SBSIZE 1024

#include "obsolete/prsem.h"

static char stdinBuf[SBSIZE];
static char stdoutBuf[SBSIZE];

static PRUintn stdinBufIdx = 0;
static PRUintn stdoutBufIdx = 0;
static PRStatus finalResult = PR_SUCCESS;


static size_t dread (PRUintn device, char *buf, size_t bufSize)
{
	PRUintn	i;
	
	
	if (stdinBufIdx == 0) {
		for (i=0; i<SBSIZE; i++)
			stdinBuf[i] = i;
	}

	
	for (i=0; i<bufSize; i++) {
		if (stdinBufIdx == SBSIZE)
			break;
		buf[i] = stdinBuf[stdinBufIdx++];
	}

	return i;
}

static size_t dwrite (PRUintn device, char *buf, size_t bufSize)
{
	PRUintn	i, j;
	
	
	for (i=0; i<bufSize; i++) {
		if (stdoutBufIdx == SBSIZE)
			break;
		stdoutBuf[stdoutBufIdx++] = buf[i];
	}

	
	if (stdoutBufIdx == SBSIZE)
		for (j=0; j<SBSIZE; j++)
			if (stdinBuf[j] != stdoutBuf[j]) {
				if (debug_mode) printf("data mismatch for index= %d \n", j);
				finalResult = PR_FAILURE;
			}

	return i;
}









PRSemaphore	*emptyBufs;	
PRSemaphore *fullBufs;	

#define BSIZE	100

struct {
	char data[BSIZE];
	PRUintn nbytes;		
} buf[2];

static void PR_CALLBACK reader(void *arg)
{
	PRUintn	i = 0;
	size_t	nbytes;
	
	do {
		(void) PR_WaitSem(emptyBufs);
		nbytes = dread(0, buf[i].data, BSIZE);
		buf[i].nbytes = nbytes;
		PR_PostSem(fullBufs);
		i = (i + 1) % 2;
	} while (nbytes > 0);
}

static void writer(void)
{
	PRUintn	i = 0;
	size_t	nbytes;
	
	do {
		(void) PR_WaitSem(fullBufs);
		nbytes = buf[i].nbytes;
		if (nbytes > 0) {
			nbytes = dwrite(1, buf[i].data, nbytes);
			PR_PostSem(emptyBufs);
			i = (i + 1) % 2;
		}
	} while (nbytes > 0);
}

int main(int argc, char **argv)
{
	PRThread *r;

    PR_STDIO_INIT();
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);

    {
    	





    	PLOptStatus os;
    	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
    	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
    		if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'd':  
    			debug_mode = 1;
                break;
             default:
                break;
            }
        }
    	PL_DestroyOptState(opt);
    }        

 

    emptyBufs = PR_NewSem(2);	

    fullBufs = PR_NewSem(0);	

	
	
	r = PR_CreateThread(PR_USER_THREAD,
				      reader, 0, 
				      PR_PRIORITY_NORMAL,
				      PR_LOCAL_THREAD,
    				  PR_UNJOINABLE_THREAD,
				      0);

	
	writer();

	PR_DestroySem(emptyBufs);
	PR_DestroySem(fullBufs);

	if (finalResult == PR_SUCCESS) {
		if (debug_mode) printf("sem Test Passed.\n");
	}
	else{
		if (debug_mode) printf("sem Test Failed.\n");
		failed_already=1;
	}
    PR_Cleanup();
	if(failed_already)	
		return 1;
	else
		return 0;
}
