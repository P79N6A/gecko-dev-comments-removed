





































#include "pk11func.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsKeygenThread.h"
#include "nsIObserver.h"
#include "nsNSSShutDown.h"
#include "PSMRunnable.h"

using namespace mozilla;
using namespace mozilla::psm;

NS_IMPL_THREADSAFE_ISUPPORTS1(nsKeygenThread, nsIKeygenThread)


nsKeygenThread::nsKeygenThread()
:mutex("nsKeygenThread.mutex"),
 iAmRunning(false),
 keygenReady(false),
 statusDialogClosed(false),
 alreadyReceivedParams(false),
 privateKey(nsnull),
 publicKey(nsnull),
 slot(nsnull),
 keyGenMechanism(0),
 params(nsnull),
 isPerm(false),
 isSensitive(false),
 wincx(nsnull),
 threadHandle(nsnull)
{
}

nsKeygenThread::~nsKeygenThread()
{
}

void nsKeygenThread::SetParams(
    PK11SlotInfo *a_slot,
    PRUint32 a_keyGenMechanism,
    void *a_params,
    bool a_isPerm,
    bool a_isSensitive,
    void *a_wincx )
{
  nsNSSShutDownPreventionLock locker;
  MutexAutoLock lock(mutex);
 
    if (!alreadyReceivedParams) {
      alreadyReceivedParams = true;
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
}

nsresult nsKeygenThread::GetParams(
    SECKEYPrivateKey **a_privateKey,
    SECKEYPublicKey **a_publicKey)
{
  if (!a_privateKey || !a_publicKey) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;

  MutexAutoLock lock(mutex);
  
    
    
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
  
  return rv;
}

static void PR_CALLBACK nsKeygenThreadRunner(void *arg)
{
  nsKeygenThread *self = static_cast<nsKeygenThread *>(arg);
  self->Run();
}

nsresult nsKeygenThread::StartKeyGeneration(nsIObserver* aObserver)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsKeygenThread::StartKeyGeneration called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }
  
  if (!aObserver)
    return NS_OK;

  MutexAutoLock lock(mutex);

    if (iAmRunning || keygenReady) {
      return NS_OK;
    }

    
    
    mNotifyObserver = new NotifyObserverRunnable(aObserver, "keygen-finished");

    iAmRunning = true;

    threadHandle = PR_CreateThread(PR_USER_THREAD, nsKeygenThreadRunner, static_cast<void*>(this), 
      PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);

    
    
    NS_ASSERTION(threadHandle, "Could not create nsKeygenThreadRunner thread\n");
  
  return NS_OK;
}

nsresult nsKeygenThread::UserCanceled(bool *threadAlreadyClosedDialog)
{
  if (!threadAlreadyClosedDialog)
    return NS_OK;

  *threadAlreadyClosedDialog = false;

  MutexAutoLock lock(mutex);
  
    if (keygenReady)
      *threadAlreadyClosedDialog = statusDialogClosed;

    
    
    
    
    statusDialogClosed = true;

  return NS_OK;
}

void nsKeygenThread::Run(void)
{
  nsNSSShutDownPreventionLock locker;
  bool canGenerate = false;

  {
    MutexAutoLock lock(mutex);
    if (alreadyReceivedParams) {
      canGenerate = true;
      keygenReady = false;
    }
  }

  if (canGenerate)
    privateKey = PK11_GenerateKeyPair(slot, keyGenMechanism,
                                         params, &publicKey,
                                         isPerm, isSensitive, wincx);
  
  
  
  
  
  

  nsCOMPtr<nsIRunnable> notifyObserver;
  {
    MutexAutoLock lock(mutex);

    keygenReady = true;
    iAmRunning = false;

    
    if (slot) {
      PK11_FreeSlot(slot);
      slot = 0;
    }
    keyGenMechanism = 0;
    params = 0;
    wincx = 0;

    if (!statusDialogClosed && mNotifyObserver)
      notifyObserver = mNotifyObserver;

    mNotifyObserver = nsnull;
  }

  if (notifyObserver) {
    nsresult rv = NS_DispatchToMainThread(notifyObserver);
    NS_ASSERTION(NS_SUCCEEDED(rv),
		 "failed to dispatch keygen thread observer to main thread");
  }
}

void nsKeygenThread::Join()
{
  if (!threadHandle)
    return;
  
  PR_JoinThread(threadHandle);
  threadHandle = nsnull;

  return;
}
