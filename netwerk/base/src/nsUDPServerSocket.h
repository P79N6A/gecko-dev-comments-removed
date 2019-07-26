




#ifndef nsUDPServerSocket_h__
#define nsUDPServerSocket_h__

#include "nsIUDPServerSocket.h"
#include "nsSocketTransportService2.h"
#include "mozilla/Mutex.h"
#include "nsIOutputStream.h"



class nsUDPServerSocket : public nsASocketHandler
                        , public nsIUDPServerSocket
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUDPSERVERSOCKET

  
  virtual void OnSocketReady(PRFileDesc* fd, int16_t outFlags);
  virtual void OnSocketDetached(PRFileDesc* fd);
  virtual void IsLocal(bool* aIsLocal);

  uint64_t ByteCountSent() { return mByteWriteCount; }
  uint64_t ByteCountReceived() { return mByteReadCount; }

  void AddOutputBytes(uint64_t aBytes);

  nsUDPServerSocket();

  
  virtual ~nsUDPServerSocket();

private:
  void OnMsgClose();
  void OnMsgAttach();

  
  nsresult TryAttach();

  
  
  mozilla::Mutex                       mLock;
  PRFileDesc                           *mFD;
  mozilla::net::NetAddr                mAddr;
  nsCOMPtr<nsIUDPServerSocketListener> mListener;
  nsCOMPtr<nsIEventTarget>             mListenerTarget;
  bool                                 mAttached;
  nsRefPtr<nsSocketTransportService>   mSts;

  uint64_t   mByteReadCount;
  uint64_t   mByteWriteCount;
};



class nsUDPMessage : public nsIUDPMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUDPMESSAGE

  nsUDPMessage(PRNetAddr* aAddr,
               nsIOutputStream* aOutputStream,
               const nsACString& aData);

private:
  virtual ~nsUDPMessage();

  PRNetAddr mAddr;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCString mData;
};




class nsUDPOutputStream : public nsIOutputStream
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM

  nsUDPOutputStream(nsUDPServerSocket* aServer,
                    PRFileDesc* aFD,
                    PRNetAddr& aPrClientAddr);
  virtual ~nsUDPOutputStream();

private:
  nsRefPtr<nsUDPServerSocket> mServer;
  PRFileDesc                  *mFD;
  PRNetAddr                   mPrClientAddr;
  bool                        mIsClosed;
};

#endif 
