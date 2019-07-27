





#include "EMEVideoDecoder.h"
#include "GMPVideoEncodedFrameImpl.h"
#include "mozilla/CDMProxy.h"

namespace mozilla {

void
EMEVideoCallbackAdapter::Error(GMPErr aErr)
{
  if (aErr == GMPNoKeyErr) {
    
    
    NS_WARNING("GMP failed to decrypt due to lack of key");
    return;
  }
  VideoCallbackAdapter::Error(aErr);
}

void
EMEVideoDecoder::InitTags(nsTArray<nsCString>& aTags)
{
  GMPVideoDecoder::InitTags(aTags);
  aTags.AppendElement(NS_ConvertUTF16toUTF8(mProxy->KeySystem()));
}

nsCString
EMEVideoDecoder::GetNodeId()
{
  return mProxy->GetNodeId();
}

GMPUnique<GMPVideoEncodedFrame>::Ptr
EMEVideoDecoder::CreateFrame(mp4_demuxer::MP4Sample* aSample)
{
  GMPUnique<GMPVideoEncodedFrame>::Ptr frame = GMPVideoDecoder::CreateFrame(aSample);
  if (frame && aSample->crypto.valid) {
    static_cast<gmp::GMPVideoEncodedFrameImpl*>(frame.get())->InitCrypto(aSample->crypto);
  }
  return frame;
}

} 
