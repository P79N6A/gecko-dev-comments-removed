




































#ifndef _NSSSLTHREAD_H_
#define _NSSSLTHREAD_H_

#include "nsCOMPtr.h"
#include "nsIRequest.h"
#include "nsPSMBackgroundThread.h"

class nsNSSSocketInfo;
class nsIHttpChannel;

class nsSSLThread : public nsPSMBackgroundThread
{
private:
  
  
  
  
  

  
  
  
  
  

  
  
  
  nsNSSSocketInfo *mBusySocket;
  
  
  
  
  
  
  
  nsNSSSocketInfo *mSocketScheduledToBeDestroyed;

  
  
  
  
  
  
  nsCOMPtr<nsIRequest> mPendingHTTPRequest;

  virtual void Run(void);

  
  static PRInt32 checkHandshake(PRInt32 bytesTransfered, 
                                PRBool wasReading,
                                PRFileDesc* fd, 
                                nsNSSSocketInfo *socketInfo);

  
  
  static void restoreOriginalSocket_locked(nsNSSSocketInfo *si);

  
  
  static PRFileDesc *getRealSSLFD(nsNSSSocketInfo *si);

  
  
  
  
  
  
  
  
  
  
  static PRStatus getRealFDIfBlockingSocket_locked(nsNSSSocketInfo *si, 
                                                   PRFileDesc *&out_fd);
public:
  nsSSLThread();
  ~nsSSLThread();

  static nsSSLThread *ssl_thread_singleton;

  
  

  static PRInt32 requestRead(nsNSSSocketInfo *si, 
                             void *buf, 
                             PRInt32 amount,
                             PRIntervalTime timeout);

  static PRInt32 requestWrite(nsNSSSocketInfo *si, 
                              const void *buf, 
                              PRInt32 amount,
                              PRIntervalTime timeout);

  static PRInt16 requestPoll(nsNSSSocketInfo *si, 
                             PRInt16 in_flags, 
                             PRInt16 *out_flags);

  static PRInt32 requestRecvMsgPeek(nsNSSSocketInfo *si, void *buf, PRInt32 amount,
                                    PRIntn flags, PRIntervalTime timeout);

  static PRStatus requestClose(nsNSSSocketInfo *si);

  static PRStatus requestGetsockname(nsNSSSocketInfo *si, PRNetAddr *addr);

  static PRStatus requestGetpeername(nsNSSSocketInfo *si, PRNetAddr *addr);

  static PRStatus requestGetsocketoption(nsNSSSocketInfo *si, 
                                         PRSocketOptionData *data);

  static PRStatus requestSetsocketoption(nsNSSSocketInfo *si, 
                                         const PRSocketOptionData *data);

  static PRStatus requestConnectcontinue(nsNSSSocketInfo *si, 
                                         PRInt16 out_flags);

  static nsresult requestActivateSSL(nsNSSSocketInfo *si);
  
  static PRBool stoppedOrStopping();
};

#endif 
