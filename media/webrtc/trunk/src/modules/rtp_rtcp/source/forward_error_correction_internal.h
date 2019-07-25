









#include "typedefs.h"

namespace webrtc {


static const int kMaskSizeLBitSet = 6;

static const int kMaskSizeLBitClear = 2;

namespace internal {

 


















void GeneratePacketMasks(int numMediaPackets,
                         int numFecPackets,
                         int numImpPackets,
                         bool useUnequalProtection,
                         uint8_t* packetMask);

} 
} 
