









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_UNIT_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_UNIT_TEST_H_

#include "../../../test_framework/unit_test.h"

class VP8UnitTest : public UnitTest
{
public:
    VP8UnitTest();
    VP8UnitTest(std::string name, std::string description);
    virtual void Perform();

protected:
    virtual WebRtc_UWord32 CodecSpecific_SetBitrate(WebRtc_UWord32 bitRate,
                                                    WebRtc_UWord32 );
    virtual bool CheckIfBitExact(const void *ptrA, unsigned int aLengthBytes,
                                 const void *ptrB, unsigned int bLengthBytes);
    static int PicIdLength(const unsigned char* ptr);
};









#endif 
