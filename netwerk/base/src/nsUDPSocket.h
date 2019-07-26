




#ifndef nsUDPSocket_h__
#define nsUDPSocket_h__

#include "nsIUDPSocket.h"
#include "mozilla/Mutex.h"
#include "nsIOutputStream.h"
#include "nsAutoPtr.h"



class nsUDPSocket : public nsASocketHandler
                  , public nsIUDPSocket
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPSOCKET

  
  virtual void OnSocketReady(PRFileDesc* fd, int16_t outFlags);
  virtual void OnSocketDetached(PRFileDesc* fd);
  virtual void IsLocal(bool* aIsLocal);

  uint64_t ByteCountSent() { return mByteWriteCount; }
  uint64_t ByteCountReceived() { return mByteReadCount; }

  void AddOutputBytes(uint64_t aBytes);

  nsUDPSocket();

  
  virtual ~nsUDPSocket();

private:
  void OnMsgClose();
  void OnMsgAttach();

  
  nsresult TryAttach();

  
  
  mozilla::Mutex                       mLock;
  PRFileDesc                           *mFD;
  mozilla::net::NetAddr                mAddr;
  nsCOMPtr<nsIUDPSocketListener>       mListener;
  nsCOMPtr<nsIEventTarget>             mListenerTarget;
  bool                                 mAttached;
  nsRefPtr<nsSocketTransportService>   mSts;

  uint64_t   mByteReadCount;
  uint64_t   mByteWriteCount;
};



class nsUDPMessage : public nsIUDPMessage
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
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
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM

  nsUDPOutputStream(nsUDPSocket* aSocket,
                    PRFileDesc* aFD,
                    PRNetAddr& aPrClientAddr);
  virtual ~nsUDPOutputStream();

private:
  nsRefPtr<nsUDPSocket>       mSocket;
  PRFileDesc                  *mFD;
  PRNetAddr                   mPrClientAddr;
  bool                        mIsClosed;
};

#endif 
