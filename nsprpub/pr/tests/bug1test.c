










































#include "prthread.h"
#include "prtypes.h"
#include "prinit.h"
#include "prmon.h"
#include "prlog.h"

typedef struct Arg_s
{
	PRInt32 a, b;
} Arg_t;

PRMonitor*  gMonitor;       
PRInt32     gReading;       
PRInt32     gWriteWaiting;  
PRInt32     gReadWaiting;   

PRInt32     gCounter;       

                            
PRInt32     gReads;         
PRInt32     gMaxReads;      
PRInt32     gMaxWriteWaits; 
PRInt32     gMaxReadWaits;  


void spin (PRInt32 aDelay)
{
  PRInt32 index;
  PRInt32 delay = aDelay * 1000;

  PR_Sleep(0);

  
  delay = (delay / 2) + (PRInt32)((float)delay *
	  ((float)rand () / (float)RAND_MAX));

  for (index = 0; index < delay * 10; index++)
	  
    ;
  PR_Sleep(0); 
}

void  doWriteThread (void* arg)
{
  PRInt32 last;
  Arg_t *args = (Arg_t*)arg;
  PRInt32 aWorkDelay = args->a, aWaitDelay = args->b;
  PR_Sleep(0);

  while (1)
  {
    
    PR_EnterMonitor (gMonitor);

    if (0 < gReading)     
    {
      PRIntervalTime fiveSecs = PR_SecondsToInterval(5);

      gWriteWaiting++;
      if (gWriteWaiting > gMaxWriteWaits) 
        gMaxWriteWaits = gWriteWaiting;
      while (0 < gReading)
        PR_Wait (gMonitor, fiveSecs);
      gWriteWaiting--;
    }
    

    last = gCounter;
    gCounter++;

    spin (aWorkDelay);

    PR_ASSERT (gCounter == (last + 1)); 

    

      PR_NotifyAll (gMonitor);

    PR_ExitMonitor (gMonitor);
    

    spin (aWaitDelay);
  }
}

void  doReadThread (void* arg)
{
  PRInt32 last;
  Arg_t *args = (Arg_t*)arg;
  PRInt32 aWorkDelay = args->a, aWaitDelay = args->b;
  PR_Sleep(0);

  while (1)
  {
    
    PR_EnterMonitor (gMonitor); 

    if (0 < gWriteWaiting)  
    {
      PRIntervalTime fiveSecs = PR_SecondsToInterval(5);

      gReadWaiting++;
      if (gReadWaiting > gMaxReadWaits) 
        gMaxReadWaits = gReadWaiting;
      while (0 < gWriteWaiting)
        PR_Wait (gMonitor, fiveSecs);
      gReadWaiting--;
    }

    gReading++;

    gReads++;   
    if (gReading > gMaxReads) 
      gMaxReads = gReading;

    PR_ExitMonitor (gMonitor);
    

    last = gCounter;

    spin (aWorkDelay);

    PR_ASSERT (gCounter == last); 

    
    PR_EnterMonitor (gMonitor);  
    gReading--;


      PR_NotifyAll (gMonitor);
    PR_ExitMonitor (gMonitor);
    

    spin (aWaitDelay);
  }
}


void fireThread (
    char* aName, void (*aProc)(void *arg), Arg_t *aArg)
{
  PRThread *thread = PR_CreateThread(
	  PR_USER_THREAD, aProc, aArg, PR_PRIORITY_NORMAL,
	  PR_LOCAL_THREAD, PR_UNJOINABLE_THREAD, 0);
}

int pseudoMain (int argc, char** argv, char *pad)
{
  PRInt32 lastWriteCount  = gCounter;
  PRInt32 lastReadCount   = gReads;
  Arg_t a1 = {500, 250};
  Arg_t a2 = {500, 500};
  Arg_t a3 = {250, 500};
  Arg_t a4 = {750, 250};
  Arg_t a5 = {100, 750};
  Arg_t a6 = {100, 500};
  Arg_t a7 = {100, 750};

  gMonitor = PR_NewMonitor ();

  fireThread ("R1", doReadThread,   &a1);
  fireThread ("R2", doReadThread,   &a2);
  fireThread ("R3", doReadThread,   &a3);
  fireThread ("R4", doReadThread,   &a4);

  fireThread ("W1", doWriteThread,  &a5);
  fireThread ("W2", doWriteThread,  &a6);
  fireThread ("W3", doWriteThread,  &a7);

  fireThread ("R5", doReadThread,   &a1);
  fireThread ("R6", doReadThread,   &a2);
  fireThread ("R7", doReadThread,   &a3);
  fireThread ("R8", doReadThread,   &a4);

  fireThread ("W4", doWriteThread,  &a5);
  fireThread ("W5", doWriteThread,  &a6);
  fireThread ("W6", doWriteThread,  &a7);
  
  while (1)
  {
	PRInt32 writeCount, readCount;
    PRIntervalTime fiveSecs = PR_SecondsToInterval(5);
    PR_Sleep (fiveSecs);  

    
    writeCount = gCounter;
    readCount   = gReads;
    printf ("\ntick %d writes (+%d), %d reads (+%d) [max %d, %d, %d]", 
            writeCount, writeCount - lastWriteCount,
            readCount, readCount - lastReadCount, 
            gMaxReads, gMaxWriteWaits, gMaxReadWaits);
    lastWriteCount = writeCount;
    lastReadCount = readCount;
    gMaxReads = gMaxWriteWaits = gMaxReadWaits = 0;
  }
  return 0;
}


static void padStack (int argc, char** argv)
{
  char pad[512];      
  pseudoMain (argc, argv, pad);
}

int main(int argc, char **argv)
{
  PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
  PR_STDIO_INIT();
  padStack (argc, argv);
}



