




#ifndef nsServerSocket_h__
#define nsServerSocket_h__

#include "nsASocketHandler.h"
#include "nsIServerSocket.h"
#include "mozilla/Mutex.h"



class nsServerSocket : public nsASocketHandler
                     , public nsIServerSocket
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISERVERSOCKET

  
  virtual void OnSocketReady(PRFileDesc *fd, int16_t outFlags);
  virtual void OnSocketDetached(PRFileDesc *fd);
  virtual void IsLocal(bool *aIsLocal);
  virtual void KeepWhenOffline(bool *aKeepWhenOffline);

  virtual uint64_t ByteCountSent() { return 0; }
  virtual uint64_t ByteCountReceived() { return 0; }
  nsServerSocket();

private:
  virtual ~nsServerSocket();

  void OnMsgClose();
  void OnMsgAttach();
  
  
  nsresult TryAttach();

  
  mozilla::Mutex                    mLock;
  PRFileDesc                       *mFD;
  PRNetAddr                         mAddr;
  nsCOMPtr<nsIServerSocketListener> mListener;
  nsCOMPtr<nsIEventTarget>          mListenerTarget;
  bool                              mAttached;
  bool                              mKeepWhenOffline;
};



#endif 
