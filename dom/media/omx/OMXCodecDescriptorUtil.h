




#ifndef OMXCodecDescriptorUtil_h_
#define OMXCodecDescriptorUtil_h_

#include <stagefright/foundation/AMessage.h>
#include <stagefright/MediaErrors.h>

#include <nsTArray.h>

#include "OMXCodecWrapper.h"

namespace android {




status_t GenerateAVCDescriptorBlob(sp<AMessage>& aConfigData,
                                   nsTArray<uint8_t>* aOutputBuf,
                                   OMXVideoEncoder::BlobFormat aFormat);

}

#endif 
