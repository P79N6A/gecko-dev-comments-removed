




#ifndef nsUDPSocket_h__
#define nsUDPSocket_h__

#include "nsIUDPSocket.h"
#include "mozilla/Mutex.h"
#include "nsIOutputStream.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"

#ifdef MOZ_WIDGET_GONK
#include "nsINetworkManager.h"
#endif



class nsUDPSocket final : public nsASocketHandler
                        , public nsIUDPSocket
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPSOCKET

  
  virtual void OnSocketReady(PRFileDesc* fd, int16_t outFlags) override;
  virtual void OnSocketDetached(PRFileDesc* fd) override;
  virtual void IsLocal(bool* aIsLocal) override;

  uint64_t ByteCountSent() override { return mByteWriteCount; }
  uint64_t ByteCountReceived() override { return mByteReadCount; }

  void AddOutputBytes(uint64_t aBytes);

  nsUDPSocket();

private:
  virtual ~nsUDPSocket();

  void OnMsgClose();
  void OnMsgAttach();

  
  nsresult TryAttach();

  friend class SetSocketOptionRunnable;
  nsresult SetSocketOption(const PRSocketOptionData& aOpt);
  nsresult JoinMulticastInternal(const PRNetAddr& aAddr,
                                 const PRNetAddr& aIface);
  nsresult LeaveMulticastInternal(const PRNetAddr& aAddr,
                                  const PRNetAddr& aIface);
  nsresult SetMulticastInterfaceInternal(const PRNetAddr& aIface);

  void SaveNetworkStats(bool aEnforce);

  
  
  mozilla::Mutex                       mLock;
  PRFileDesc                           *mFD;
  mozilla::net::NetAddr                mAddr;
  uint32_t                             mAppId;
  bool                                 mIsInBrowserElement;
  nsCOMPtr<nsIUDPSocketListener>       mListener;
  nsCOMPtr<nsIEventTarget>             mListenerTarget;
  bool                                 mAttached;
  nsRefPtr<nsSocketTransportService>   mSts;

  uint64_t   mByteReadCount;
  uint64_t   mByteWriteCount;
#ifdef MOZ_WIDGET_GONK
  nsMainThreadPtrHandle<nsINetworkInterface> mActiveNetwork;
#endif
};



class nsUDPMessage : public nsIUDPMessage
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsUDPMessage)
  NS_DECL_NSIUDPMESSAGE

  nsUDPMessage(mozilla::net::NetAddr* aAddr,
               nsIOutputStream* aOutputStream,
               FallibleTArray<uint8_t>& aData);

private:
  virtual ~nsUDPMessage();

  mozilla::net::NetAddr mAddr;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  FallibleTArray<uint8_t> mData;
  JS::Heap<JSObject*> mJsobj;
};




class nsUDPOutputStream : public nsIOutputStream
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM

  nsUDPOutputStream(nsUDPSocket* aSocket,
                    PRFileDesc* aFD,
                    PRNetAddr& aPrClientAddr);

private:
  virtual ~nsUDPOutputStream();

  nsRefPtr<nsUDPSocket>       mSocket;
  PRFileDesc                  *mFD;
  PRNetAddr                   mPrClientAddr;
  bool                        mIsClosed;
};

#endif 
