









#ifndef TEST_VAD_DTX_H
#define TEST_VAD_DTX_H

#include "ACMTest.h"
#include "Channel.h"
#include "PCMFile.h"

namespace webrtc {

typedef struct 
{
    bool statusDTX;
    bool statusVAD;
    ACMVADMode vadMode;
} VADDTXstruct;

class ActivityMonitor : public ACMVADCallback
{
public:
    ActivityMonitor();
    ~ActivityMonitor();
    WebRtc_Word32 InFrameType(WebRtc_Word16 frameType);
    void PrintStatistics(int testMode);
    void ResetStatistics();
    void GetStatistics(WebRtc_UWord32* getCounter);
private:
    
    








    WebRtc_UWord32 _counter[6];
};

class TestVADDTX : public ACMTest
{
public:
    TestVADDTX(int testMode);
    ~TestVADDTX();

    void Perform();
private:
    
    
    WebRtc_Word16 RegisterSendCodec(char side, 
        char* codecName, 
        WebRtc_Word32 samplingFreqHz = -1,
        WebRtc_Word32 rateKhz = -1);
    void Run();
    void OpenOutFile(WebRtc_Word16 testNumber);
    void runTestCases();
    void runTestInternalDTX();
    void SetVAD(bool statusDTX, bool statusVAD, WebRtc_Word16 vadMode);
    VADDTXstruct GetVAD();
    WebRtc_Word16 VerifyTest();
    AudioCodingModule* _acmA;
    AudioCodingModule* _acmB;

    Channel*               _channelA2B;

    PCMFile                _inFileA;
    PCMFile                _outFileB;

    ActivityMonitor        _monitor;
    WebRtc_UWord32           _statCounter[6];

    int                    _testMode;
    int                    _testResults;
    VADDTXstruct           _setStruct;
    VADDTXstruct           _getStruct;
};

} 

#endif
