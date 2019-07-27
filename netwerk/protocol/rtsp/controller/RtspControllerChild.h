





#ifndef RtspControllerChild_h
#define RtspControllerChild_h

#include "mozilla/net/PRtspControllerChild.h"
#include "nsIStreamingProtocolController.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/net/RtspChannelChild.h"
#include "mozilla/Mutex.h"
#include "nsITimer.h"

namespace mozilla {
namespace net {

class RtspControllerChild : public nsIStreamingProtocolController
                          , public nsIStreamingProtocolListener
                          , public PRtspControllerChild
{
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMINGPROTOCOLCONTROLLER
  NS_DECL_NSISTREAMINGPROTOCOLLISTENER

  RtspControllerChild(nsIChannel *channel);

  bool RecvOnConnected(const uint8_t& index,
                       InfallibleTArray<RtspMetadataParam>&& meta);

  bool RecvOnMediaDataAvailable(
         const uint8_t& index,
         const nsCString& data,
         const uint32_t& length,
         const uint32_t& offset,
         InfallibleTArray<RtspMetadataParam>&& meta);

  bool RecvOnDisconnected(const uint8_t& index,
                          const nsresult& reason);

  bool RecvAsyncOpenFailed(const nsresult& reason);
  void AddIPDLReference();
  void ReleaseIPDLReference();
  void AddMetaData(already_AddRefed<nsIStreamingProtocolMetaData>&& meta);
  int  GetMetaDataLength();
  bool OKToSendIPC();
  void AllowIPC();
  void DisallowIPC();

  
  static void PlayTimerCallback(nsITimer *aTimer, void *aClosure);
  static void PauseTimerCallback(nsITimer *aTimer, void *aClosure);

 protected:
  ~RtspControllerChild();

 private:
  bool mIPCOpen;
  
  
  bool mIPCAllowed;
  
  nsCOMPtr<nsIChannel> mChannel;
  
  nsCOMPtr<nsIStreamingProtocolListener> mListener;
  
  nsCOMPtr<nsIURI> mURI;
  
  nsTArray<nsCOMPtr<nsIStreamingProtocolMetaData>> mMetaArray;
  
  nsCString mSpec;
  
  uint32_t mTotalTracks;
  
  uint32_t mSuspendCount;
  
  void ReleaseChannel();
  
  Mutex mTimerLock;
  
  
  
  nsCOMPtr<nsITimer> mPlayTimer;
  nsCOMPtr<nsITimer> mPauseTimer;
  
  
  void StopPlayAndPauseTimer();
};

} 
} 

#endif 
