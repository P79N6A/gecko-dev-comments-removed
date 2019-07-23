




































#include "prmon.h"
#include "plhash.h"
#include "nsCOMPtr.h"
#include "nsAppShell.h"
#include "nsIAppShell.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"

#include <stdlib.h>

#include "nsIWidget.h"
#include "nsCRT.h"

#include <Pt.h>
#include <errno.h>


PRBool nsAppShell::gExitMainLoop = PR_FALSE;

static PLHashTable *sQueueHashTable = nsnull;
static PLHashTable *sCountHashTable = nsnull;


PRBool nsAppShell::mPtInited = PR_FALSE;






static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);






nsAppShell::nsAppShell()  
{
  mEventQueue  = nsnull;
  mFD          = -1;
}






nsAppShell::~nsAppShell()
{
  if (mFD != -1)
  {
    int err=PtAppRemoveFd(NULL,mFD);

    if (err==-1)
    {
	  NS_WARNING("nsAppShell::~EventQueueTokenQueue Run Error calling PtAppRemoveFd");
#ifdef DEBUG
	  printf("nsAppShell::~EventQueueTokenQueue Run Error calling PtAppRemoveFd mFD=<%d> errno=<%d>\n", mFD, errno);
#endif
    }  
    mFD = -1;
  }
}







NS_IMPL_ISUPPORTS1(nsAppShell, nsIAppShell)







static int event_processor_callback(int fd, void *data, unsigned mode)
{
	nsIEventQueue *eventQueue = (nsIEventQueue*)data;
	PtHold();
	if (eventQueue)
	   eventQueue->ProcessPendingEvents();
	PtRelease();
  return Pt_CONTINUE;
}








NS_IMETHODIMP nsAppShell::Create(int *bac, char **bav)
{
	




	if( !mPtInited )
	{
		PtInit( NULL );
		PtChannelCreate(); 
		mPtInited = PR_TRUE;
	}

  return NS_OK;
}






NS_METHOD nsAppShell::Spinup()
{
  nsresult   rv = NS_OK;

  
  nsCOMPtr<nsIEventQueueService> eventQService = do_GetService(kEventQueueServiceCID, &rv);

  if (NS_FAILED(rv)) {
    NS_ASSERTION("Could not obtain event queue service", PR_FALSE);
    return rv;
  }

  
	rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(mEventQueue));

  
  if (mEventQueue)
    goto done;

  
  rv = eventQService->CreateThreadEventQueue();
  if (NS_FAILED(rv)) {
    NS_ASSERTION("Could not create the thread event queue", PR_FALSE);
    return rv;
  }

  
	rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(mEventQueue));

  
 done:
  ListenToEventQueue(mEventQueue, PR_TRUE);

  return rv;
}






NS_METHOD nsAppShell::Spindown()
{
  if (mEventQueue) {
    ListenToEventQueue(mEventQueue, PR_FALSE);
    mEventQueue->ProcessPendingEvents();
    mEventQueue = nsnull;
  }

  return NS_OK;
}





void MyMainLoop( void ) 
{
	nsAppShell::gExitMainLoop = PR_FALSE;
	while (! nsAppShell::gExitMainLoop)
	{
		PtProcessEvent();
	}

#ifdef DEBUG
    printf("nsAppShell: MyMainLoop exiting!\n");
#endif
}






NS_IMETHODIMP nsAppShell::Run()
{
  if (!mEventQueue)
    Spinup();

  if (!mEventQueue)
    return NS_ERROR_NOT_INITIALIZED;

  
  MyMainLoop();

  Spindown();

  return NS_OK; 
}







NS_METHOD nsAppShell::Exit()
{
  gExitMainLoop = PR_TRUE;
  return NS_OK;
}


#define NUMBER_HASH_KEY(_num) ((PLHashNumber) _num)

static PLHashNumber
IntHashKey(PRInt32 key)
{
  return NUMBER_HASH_KEY(key);
}

NS_IMETHODIMP nsAppShell::ListenToEventQueue(nsIEventQueue *aQueue,
                                             PRBool aListen)
{

  if (!sQueueHashTable) {
    sQueueHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
                                      PL_CompareValues, PL_CompareValues, 0, 0);
  }
  if (!sCountHashTable) {
    sCountHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
                                      PL_CompareValues, PL_CompareValues, 0, 0);
  }

  if (aListen) {
    
    PRInt32 key = aQueue->GetEventQueueSelectFD();

    
    if (!PL_HashTableLookup(sQueueHashTable, (void *)(key))) {
			PRInt32 tag = PtAppAddFd( NULL, aQueue->GetEventQueueSelectFD(), (Pt_FD_READ | Pt_FD_NOPOLL | Pt_FD_DRAIN ),
							event_processor_callback, aQueue );

      if (tag >= 0) {
        PL_HashTableAdd(sQueueHashTable, (void *)(key), (void *)(key));
      }
    }
    
    int count = (int)(PL_HashTableLookup(sCountHashTable, (void *)(key)));
    PL_HashTableAdd(sCountHashTable, (void *)(key), (void *)(count+1));
  } else {
    
    PRInt32 key = aQueue->GetEventQueueSelectFD();

    int count = (int)(PL_HashTableLookup(sCountHashTable, (void *)(key)));
    if (count - 1 == 0) {
      int tag = (int)(PL_HashTableLookup(sQueueHashTable, (void *)(key)));
      if (tag > 0) {
      	PtAppRemoveFd(NULL, key);
        PL_HashTableRemove(sQueueHashTable, (void *)(key));
      }
    }
    PL_HashTableAdd(sCountHashTable, (void *)(key), (void *)(count-1));

  }

  return NS_OK;
}
