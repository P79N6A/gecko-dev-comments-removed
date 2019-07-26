





#ifndef RtspControllerChild_h
#define RtspControllerChild_h

#include "mozilla/net/PRtspControllerChild.h"
#include "nsIStreamingProtocolController.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"

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
  ~RtspControllerChild();

  bool RecvOnConnected(const uint8_t& index,
                       const InfallibleTArray<RtspMetadataParam>& meta);

  bool RecvOnMediaDataAvailable(
         const uint8_t& index,
         const nsCString& data,
         const uint32_t& length,
         const uint32_t& offset,
         const InfallibleTArray<RtspMetadataParam>& meta);

  bool RecvOnDisconnected(const uint8_t& index,
                          const uint32_t& reason);

  bool RecvAsyncOpenFailed(const uint8_t& reason);
  void AddIPDLReference();
  void ReleaseIPDLReference();
  void AddMetaData(already_AddRefed<nsIStreamingProtocolMetaData> meta);
  int  GetMetaDataLength();

 private:
  bool mIPCOpen;
  
  nsCOMPtr<nsIChannel> mChannel;
  
  nsCOMPtr<nsIStreamingProtocolListener> mListener;
  
  nsCOMPtr<nsIURI> mURI;
  
  nsTArray<nsCOMPtr<nsIStreamingProtocolMetaData>> mMetaArray;
  
  nsCString mSpec;
  
  uint32_t mTotalTracks;
  
  uint32_t mSuspendCount;
};
} 
} 

#endif 
