




































#ifndef mozilla_plugins_BrowserStreamChild_h
#define mozilla_plugins_BrowserStreamChild_h 1

#include "mozilla/plugins/PBrowserStreamChild.h"
#include "mozilla/plugins/AStream.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;
class StreamNotifyChild;

class BrowserStreamChild : public PBrowserStreamChild, public AStream
{
public:
  BrowserStreamChild(PluginInstanceChild* instance,
                     const nsCString& url,
                     const uint32_t& length,
                     const uint32_t& lastmodified,
                     StreamNotifyChild* notifyData,
                     const nsCString& headers,
                     const nsCString& mimeType,
                     const bool& seekable,
                     NPError* rv,
                     uint16_t* stype);
  virtual ~BrowserStreamChild();

  NS_OVERRIDE virtual bool IsBrowserStream() { return true; }

  NPError StreamConstructed(
            const nsCString& mimeType,
            const bool& seekable,
            uint16_t* stype);

  virtual bool RecvWrite(const int32_t& offset,
                         const Buffer& data,
                         const uint32_t& newsize);
  virtual bool AnswerNPP_StreamAsFile(const nsCString& fname);
  virtual bool RecvNPP_DestroyStream(const NPReason& reason);
  virtual bool Recv__delete__();

  void EnsureCorrectInstance(PluginInstanceChild* i)
  {
    if (i != mInstance)
      NS_RUNTIMEABORT("Incorrect stream instance");
  }
  void EnsureCorrectStream(NPStream* s)
  {
    if (s != &mStream)
      NS_RUNTIMEABORT("Incorrect stream data");
  }

  NPError NPN_RequestRead(NPByteRange* aRangeList);
  void NPN_DestroyStream(NPReason reason);

  void NotifyPending() {
    NS_ASSERTION(!mNotifyPending, "Pending twice?");
    mNotifyPending = true;
    EnsureDeliveryPending();
  }

  




  bool InstanceDying() {
    if (DELETING == mState)
      return false;

    mInstanceDying = true;
    return true;
  }

  void FinishDelivery() {
    NS_ASSERTION(mInstanceDying, "Should only be called after InstanceDying");
    NS_ASSERTION(DELETING != mState, "InstanceDying didn't work?");
    mStreamStatus = NPRES_USER_BREAK;
    Deliver();
    NS_ASSERTION(!mStreamNotify, "Didn't deliver NPN_URLNotify?");
  }

private:
  friend class StreamNotifyChild;
  using PBrowserStreamChild::SendNPN_DestroyStream;

  



  void EnsureDeliveryPending();

  



  void Deliver();

  



  bool DeliverPendingData();

  void SetSuspendedTimer();
  void ClearSuspendedTimer();

  PluginInstanceChild* mInstance;
  NPStream mStream;

  static const NPReason kStreamOpen = -1;

  






  NPReason mStreamStatus;

  



  enum {
    NOT_DESTROYED, 
    DESTROY_PENDING, 
    DESTROYED 
  } mDestroyPending;
  bool mNotifyPending;

  
  
  bool mInstanceDying;

  enum {
    CONSTRUCTING,
    ALIVE,
    DYING,
    DELETING
  } mState;
  nsCString mURL;
  nsCString mHeaders;
  StreamNotifyChild* mStreamNotify;

  struct PendingData
  {
    int32_t offset;
    Buffer data;
    int32_t curpos;
  };
  nsTArray<PendingData> mPendingData;

  





  ScopedRunnableMethodFactory<BrowserStreamChild> mDeliveryTracker;
  base::RepeatingTimer<BrowserStreamChild> mSuspendedTimer;
};

} 
} 

#endif 
