





































#ifndef _NSKEYGENTHREAD_H_
#define _NSKEYGENTHREAD_H_

#include "keyhi.h"
#include "nspr.h"

#include "mozilla/Mutex.h"
#include "nsIKeygenThread.h"
#include "nsCOMPtr.h"

class nsIRunnable;

class nsKeygenThread : public nsIKeygenThread
{
private:
  mozilla::Mutex mutex;
  
  nsCOMPtr<nsIRunnable> mNotifyObserver;

  bool iAmRunning;
  bool keygenReady;
  bool statusDialogClosed;
  bool alreadyReceivedParams;

  SECKEYPrivateKey *privateKey;
  SECKEYPublicKey *publicKey;
  PK11SlotInfo *slot;
  PRUint32 keyGenMechanism;
  void *params;
  bool isPerm;
  bool isSensitive;
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
    bool a_isPerm,
    bool a_isSensitive,
    void *a_wincx );

  nsresult GetParams(
    SECKEYPrivateKey **a_privateKey,
    SECKEYPublicKey **a_publicKey);
  
  void Join(void);

  void Run(void);
};

#endif 
