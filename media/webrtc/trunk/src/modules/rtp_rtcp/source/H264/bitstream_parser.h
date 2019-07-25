









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_H264_BITSTREAM_PARSER_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_H264_BITSTREAM_PARSER_H_

#include "typedefs.h"

namespace webrtc {
class BitstreamParser
{
public:
    BitstreamParser(const WebRtc_UWord8* data, const WebRtc_UWord32 dataLength);

    WebRtc_UWord8 Get1Bit();
    WebRtc_UWord8 Get2Bits();
    WebRtc_UWord8 Get3Bits();
    WebRtc_UWord8 Get4Bits();
    WebRtc_UWord8 Get5Bits();
    WebRtc_UWord8 Get6Bits();
    WebRtc_UWord8 Get7Bits();
    WebRtc_UWord8 Get8Bits();
    WebRtc_UWord16 Get16Bits();
    WebRtc_UWord32 Get24Bits();
    WebRtc_UWord32 Get32Bits();

    
    WebRtc_UWord32 GetUE();

private:
    const WebRtc_UWord8* _data;
    const WebRtc_UWord32 _dataLength;

    WebRtc_UWord32 _byteOffset;
    WebRtc_UWord8  _bitOffset;
};
} 

#endif 
