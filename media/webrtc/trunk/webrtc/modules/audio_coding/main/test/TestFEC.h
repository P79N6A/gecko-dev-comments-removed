









#ifndef TEST_FEC_H
#define TEST_FEC_H

#include "ACMTest.h"
#include "Channel.h"
#include "PCMFile.h"

namespace webrtc {

class TestFEC : public ACMTest
{
public:
    TestFEC(int testMode);
    ~TestFEC();

    void Perform();
private:
    
    
    
    WebRtc_Word16 RegisterSendCodec(char side, char* codecName, WebRtc_Word32 sampFreqHz = -1);
    void Run();
    void OpenOutFile(WebRtc_Word16 testNumber);
    void DisplaySendReceiveCodec();
    WebRtc_Word32 SetVAD(bool enableDTX, bool enableVAD, ACMVADMode vadMode);
    AudioCodingModule* _acmA;
    AudioCodingModule* _acmB;

    Channel*               _channelA2B;

    PCMFile                _inFileA;
    PCMFile                _outFileB;
    WebRtc_Word16            _testCntr;
    int                    _testMode;
};

} 

#endif
