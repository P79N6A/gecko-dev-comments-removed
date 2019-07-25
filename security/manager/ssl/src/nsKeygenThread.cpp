





































#include "pk11func.h"
#include "nsCOMPtr.h"
#include "nsProxiedService.h"
#include "nsThreadUtils.h"
#include "nsKeygenThread.h"
#include "nsIObserver.h"
#include "nsNSSShutDown.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsKeygenThread, nsIKeygenThread)


nsKeygenThread::nsKeygenThread()
:mutex(nsnull),
 iAmRunning(PR_FALSE),
 keygenReady(PR_FALSE),
 statusDialogClosed(PR_FALSE),
 alreadyReceivedParams(PR_FALSE),
 privateKey(nsnull),
 publicKey(nsnull),
 slot(nsnull),
 keyGenMechanism(0),
 params(nsnull),
 isPerm(PR_FALSE),
 isSensitive(PR_FALSE),
 wincx(nsnull),
 threadHandle(nsnull)
{
  mutex = PR_NewLock();
}

nsKeygenThread::~nsKeygenThread()
{
  if (mutex) {
    PR_DestroyLock(mutex);
  }
}

void nsKeygenThread::SetParams(
    PK11SlotInfo *a_slot,
    PRUint32 a_keyGenMechanism,
    void *a_params,
    PRBool a_isPerm,
    PRBool a_isSensitive,
    void *a_wincx )
{
  nsNSSShutDownPreventionLock locker;
  PR_Lock(mutex);
 
    if (!alreadyReceivedParams) {
      alreadyReceivedParams = PR_TRUE;
      if (a_slot) {
        slot = PK11_ReferenceSlot(a_slot);
      }
      else {
        slot = nsnull;
      }
      keyGenMechanism = a_keyGenMechanism;
      params = a_params;
      isPerm = a_isPerm;
      isSensitive = a_isSensitive;
      wincx = a_wincx;
    }

  PR_Unlock(mutex);
}

nsresult nsKeygenThread::GetParams(
    SECKEYPrivateKey **a_privateKey,
    SECKEYPublicKey **a_publicKey)
{
  if (!a_privateKey || !a_publicKey) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  
  PR_Lock(mutex);
  
    
    
    NS_ASSERTION(keygenReady, "logic error in nsKeygenThread::GetParams");

    if (keygenReady) {
      *a_privateKey = privateKey;
      *a_publicKey = publicKey;

      privateKey = 0;
      publicKey = 0;
      
      rv = NS_OK;
    }
    else {
      rv = NS_ERROR_FAILURE;
    }
  
  PR_Unlock(mutex);
  
  return rv;
}

static void PR_CALLBACK nsKeygenThreadRunner(void *arg)
{
  nsKeygenThread *self = static_cast<nsKeygenThread *>(arg);
  self->Run();
}

nsresult nsKeygenThread::StartKeyGeneration(nsIObserver* aObserver)
{
  if (!mutex)
    return NS_OK;

  if (!aObserver)
    return NS_OK;

  nsCOMPtr<nsIObserver> obs;
  NS_GetProxyForObject( NS_PROXY_TO_MAIN_THREAD,
                        NS_GET_IID(nsIObserver),
                        aObserver,
                        NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                        getter_AddRefs(obs));

  PR_Lock(mutex);

    if (iAmRunning || keygenReady) {
      PR_Unlock(mutex);
      return NS_OK;
    }

    observer.swap(obs);

    iAmRunning = PR_TRUE;

    threadHandle = PR_CreateThread(PR_USER_THREAD, nsKeygenThreadRunner, static_cast<void*>(this), 
      PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);

    
    
    NS_ASSERTION(threadHandle, "Could not create nsKeygenThreadRunner thread\n");

  PR_Unlock(mutex);
  
  return NS_OK;
}

nsresult nsKeygenThread::UserCanceled(PRBool *threadAlreadyClosedDialog)
{
  if (!threadAlreadyClosedDialog)
    return NS_OK;

  *threadAlreadyClosedDialog = PR_FALSE;

  if (!mutex)
    return NS_OK;

  PR_Lock(mutex);
  
    if (keygenReady)
      *threadAlreadyClosedDialog = statusDialogClosed;

    
    
    
    
    statusDialogClosed = PR_TRUE;

  PR_Unlock(mutex);

  return NS_OK;
}

void nsKeygenThread::Run(void)
{
  nsNSSShutDownPreventionLock locker;
  PRBool canGenerate = PR_FALSE;

  PR_Lock(mutex);

    if (alreadyReceivedParams) {
      canGenerate = PR_TRUE;
      keygenReady = PR_FALSE;
    }

  PR_Unlock(mutex);

  if (canGenerate)
    privateKey = PK11_GenerateKeyPair(slot, keyGenMechanism,
                                         params, &publicKey,
                                         isPerm, isSensitive, wincx);
  
  
  
  
  
  

  nsCOMPtr<nsIObserver> obs;
  PR_Lock(mutex);

    keygenReady = PR_TRUE;
    iAmRunning = PR_FALSE;

    
    if (slot) {
      PK11_FreeSlot(slot);
      slot = 0;
    }
    keyGenMechanism = 0;
    params = 0;
    wincx = 0;

    if (!statusDialogClosed)
      obs = observer;

    observer = nsnull;

  PR_Unlock(mutex);

  if (obs)
    obs->Observe(nsnull, "keygen-finished", nsnull);
}

void nsKeygenThread::Join()
{
  if (!threadHandle)
    return;
  
  PR_JoinThread(threadHandle);
  threadHandle = nsnull;

  return;
}
