





































#ifndef _NSKEYGENTHREAD_H_
#define _NSKEYGENTHREAD_H_

#include "keyhi.h"
#include "nspr.h"

#include "nsIKeygenThread.h"
#include "nsCOMPtr.h"

class nsIObserver;

class nsKeygenThread : public nsIKeygenThread
{
private:
  PRLock *mutex;
  
  nsCOMPtr<nsIObserver> observer;

  PRBool iAmRunning;
  PRBool keygenReady;
  PRBool statusDialogClosed;
  PRBool alreadyReceivedParams;

  SECKEYPrivateKey *privateKey;
  SECKEYPublicKey *publicKey;
  PK11SlotInfo *slot;
  PRUint32 keyGenMechanism;
  void *params;
  PRBool isPerm;
  PRBool isSensitive;
  void *wincx;

  PRThread *threadHandle;
  
public:
  nsKeygenThread();
  virtual ~nsKeygenThread();
  
  NS_DECL_NSIKEYGENTHREAD
  NS_DECL_ISUPPORTS

  void SetParams(
    PK11SlotInfo *a_slot,
    PRUint32 a_keyGenMechanism,
    void *a_params,
    PRBool a_isPerm,
    PRBool a_isSensitive,
    void *a_wincx );

  nsresult GetParams(
    SECKEYPrivateKey **a_privateKey,
    SECKEYPublicKey **a_publicKey);
  
  void Join(void);

  void Run(void);
};

#endif 
