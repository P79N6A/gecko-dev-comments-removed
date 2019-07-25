




































#ifndef nsServerSocket_h__
#define nsServerSocket_h__

#include "nsIServerSocket.h"
#include "nsSocketTransportService2.h"



class nsServerSocket : public nsASocketHandler
                     , public nsIServerSocket
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERVERSOCKET

  
  virtual void OnSocketReady(PRFileDesc *fd, PRInt16 outFlags);
  virtual void OnSocketDetached(PRFileDesc *fd);

  nsServerSocket();

  
  virtual ~nsServerSocket();

private:
  void OnMsgClose();
  void OnMsgAttach();
  
  
  nsresult TryAttach();

  
  PRLock                           *mLock;
  PRFileDesc                       *mFD;
  PRNetAddr                         mAddr;
  nsCOMPtr<nsIServerSocketListener> mListener;
  nsCOMPtr<nsIEventTarget>          mListenerTarget;
  PRBool                            mAttached;
};



#endif 
