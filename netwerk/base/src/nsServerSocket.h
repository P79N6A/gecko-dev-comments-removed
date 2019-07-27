




#ifndef nsServerSocket_h__
#define nsServerSocket_h__

#include "prio.h"
#include "nsASocketHandler.h"
#include "nsIServerSocket.h"
#include "mozilla/Mutex.h"



class nsIEventTarget;
namespace mozilla { namespace net {
union NetAddr;
}} 

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

  virtual void CreateClientTransport(PRFileDesc* clientFD,
                                     const mozilla::net::NetAddr& clientAddr);
  virtual nsresult SetSocketDefaults() { return NS_OK; }
  virtual nsresult OnSocketListen() { return NS_OK; }

protected:
  virtual ~nsServerSocket();
  PRFileDesc*                       mFD;
  nsCOMPtr<nsIServerSocketListener> mListener;

private:
  void OnMsgClose();
  void OnMsgAttach();

  
  nsresult TryAttach();

  
  mozilla::Mutex                    mLock;
  PRNetAddr                         mAddr;
  nsCOMPtr<nsIEventTarget>          mListenerTarget;
  bool                              mAttached;
  bool                              mKeepWhenOffline;
};



#endif 
