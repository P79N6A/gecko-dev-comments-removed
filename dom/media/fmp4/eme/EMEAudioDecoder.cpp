





#include "EMEAudioDecoder.h"
#include "mozilla/CDMProxy.h"

namespace mozilla {

void
EMEAudioCallbackAdapter::Error(GMPErr aErr)
{
  if (aErr == GMPNoKeyErr) {
    
    
    NS_WARNING("GMP failed to decrypt due to lack of key");
    return;
  }
  AudioCallbackAdapter::Error(aErr);
}

void
EMEAudioDecoder::InitTags(nsTArray<nsCString>& aTags)
{
  GMPAudioDecoder::InitTags(aTags);
  aTags.AppendElement(NS_ConvertUTF16toUTF8(mProxy->KeySystem()));
}

nsCString
EMEAudioDecoder::GetNodeId()
{
  return mProxy->GetNodeId();
}

} 
