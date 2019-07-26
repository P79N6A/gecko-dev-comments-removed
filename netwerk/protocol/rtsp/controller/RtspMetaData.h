





#ifndef RtspMetaData_h
#define RtspMetaData_h

#include "mozilla/net/PRtspController.h"
#include "nsIStreamingProtocolController.h"
#include "nsCOMPtr.h"
#include "nsString.h"

namespace mozilla {
namespace net {

class RtspMetaData : public nsIStreamingProtocolMetaData
{
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMINGPROTOCOLMETADATA

  RtspMetaData();
  ~RtspMetaData();

  nsresult DeserializeRtspMetaData(const InfallibleTArray<RtspMetadataParam>& metaArray);

 private:
  uint32_t  mFrameType;
  uint32_t  mIndex;
  uint32_t  mTotalTracks;
  uint32_t  mWidth;
  uint32_t  mHeight;
  uint64_t  mDuration;
  uint32_t  mSampleRate;
  uint64_t  mCurrentTimeStamp;
  uint32_t  mChannelCount;
  nsCString mMimeType;
  nsCString mESDS;
  nsCString mAVCC;
};

} 
} 

#endif 
