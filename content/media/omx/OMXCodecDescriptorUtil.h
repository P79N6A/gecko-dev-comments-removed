




#ifndef OMXCodecDescriptorUtil_h_
#define OMXCodecDescriptorUtil_h_

#include <stagefright/foundation/ABuffer.h>
#include <stagefright/MediaErrors.h>

#include <nsTArray.h>

namespace android {



status_t GenerateAVCDescriptorBlob(ABuffer* aData,
                                   nsTArray<uint8_t>* aOutputBuf);

}

#endif 