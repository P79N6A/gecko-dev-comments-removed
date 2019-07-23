
























































#include "prinit.h"
#include "prthread.h"
#include "prlock.h"
#include "prcvar.h"
#include "prmon.h"
#include "prmem.h"
#include "prio.h"
#include "prlog.h"

#include <stdio.h>

#include "obsolete/prsem.h"


#define TBSIZE 1024

static PRUint8 tbuf[TBSIZE];

static PRFileDesc *t1, *t2;

PRIntn failed_already=0;
PRIntn debug_mode;
static void InitialSetup(void)
{
	PRUintn	i;
	PRInt32 nWritten, rv;
	
	t1 = PR_Open("t1.tmp", PR_CREATE_FILE | PR_RDWR, 0);
	PR_ASSERT(t1 != NULL);	
	
	for (i=0; i<TBSIZE; i++)
		tbuf[i] = i;
		
	nWritten = PR_Write((PRFileDesc*)t1, tbuf, TBSIZE);
	PR_ASSERT(nWritten == TBSIZE);	
   		
	rv = PR_Seek(t1,0,PR_SEEK_SET);
	PR_ASSERT(rv == 0);	

   	t2 = PR_Open("t2.tmp", PR_CREATE_FILE | PR_RDWR, 0);
	PR_ASSERT(t2 != NULL);	
}


static void VerifyAndCleanup(void)
{
	PRUintn	i;
	PRInt32 nRead, rv;
	
	for (i=0; i<TBSIZE; i++)
		tbuf[i] = 0;
		
	rv = PR_Seek(t2,0,PR_SEEK_SET);
	PR_ASSERT(rv == 0);	

	nRead = PR_Read((PRFileDesc*)t2, tbuf, TBSIZE);
	PR_ASSERT(nRead == TBSIZE);	
   		
	for (i=0; i<TBSIZE; i++)
		if (tbuf[i] != (PRUint8)i) {
			if (debug_mode) printf("data mismatch for index= %d \n", i);
			else failed_already=1;
		}
   	PR_Close(t1);
   	PR_Close(t2);

   	PR_Delete("t1.tmp");
   	PR_Delete("t2.tmp");

    if (debug_mode) printf("fileio test passed\n");
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
	PRInt32	nbytes;
	
	do {
		(void) PR_WaitSem(emptyBufs);
		nbytes = PR_Read((PRFileDesc*)arg, buf[i].data, BSIZE);
		if (nbytes >= 0) {
			buf[i].nbytes = nbytes;
			PR_PostSem(fullBufs);
			i = (i + 1) % 2;
		}
	} while (nbytes > 0);
}

static void PR_CALLBACK writer(void *arg)
{
	PRUintn	i = 0;
	PRInt32	nbytes;
	
	do {
		(void) PR_WaitSem(fullBufs);
		nbytes = buf[i].nbytes;
		if (nbytes > 0) {
			nbytes = PR_Write((PRFileDesc*)arg, buf[i].data, nbytes);
			PR_PostSem(emptyBufs);
			i = (i + 1) % 2;
		}
	} while (nbytes > 0);
}

int main(int argc, char **argv)
{
	PRThread *r, *w;


	PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    emptyBufs = PR_NewSem(2);	

    fullBufs = PR_NewSem(0);	

	
	InitialSetup();
	
	
	
	r = PR_CreateThread(PR_USER_THREAD,
				      reader, t1, 
				      PR_PRIORITY_NORMAL,
				      PR_LOCAL_THREAD,
    				  PR_JOINABLE_THREAD,
				      0);

	w = PR_CreateThread(PR_USER_THREAD,
				      writer, t2, 
				      PR_PRIORITY_NORMAL,
                      PR_LOCAL_THREAD,
                      PR_JOINABLE_THREAD,
                      0);

    
    (void) PR_JoinThread(r);
    (void) PR_JoinThread(w);

    
    VerifyAndCleanup();

    PR_DestroySem(emptyBufs);
    PR_DestroySem(fullBufs);

    PR_Cleanup();

    if(failed_already)
    {
        printf("Fail\n");
        return 1;
    }
    else
    {
        printf("PASS\n");
        return 0;
    }


}
