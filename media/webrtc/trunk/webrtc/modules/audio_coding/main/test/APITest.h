









#ifndef API_TEST_H
#define API_TEST_H

#include "ACMTest.h"
#include "Channel.h"
#include "PCMFile.h"
#include "event_wrapper.h"
#include "utility.h"

namespace webrtc {

enum APITESTAction {TEST_CHANGE_CODEC_ONLY = 0, DTX_TEST = 1};

class APITest : public ACMTest
{
public:
    APITest();
    ~APITest();

    void Perform();
private:
    WebRtc_Word16 SetUp();

    static bool PushAudioThreadA(void* obj);
    static bool PullAudioThreadA(void* obj);
    static bool ProcessThreadA(void* obj);
    static bool APIThreadA(void* obj);

    static bool PushAudioThreadB(void* obj);
    static bool PullAudioThreadB(void* obj);
    static bool ProcessThreadB(void* obj);
    static bool APIThreadB(void* obj);

    void CheckVADStatus(char side);

    
    void TestDelay(char side);

    
    void TestRegisteration(char side);

    
    
    void TestPlayout(char receiveSide);

    
    void TestReceiverVAD(char side);

    
    void TestSendVAD(char side);

    void CurrentCodec(char side);

    void ChangeCodec(char side);

    void Wait(WebRtc_UWord32 waitLengthMs);

    void LookForDTMF(char side);

    void RunTest(char thread);

    bool PushAudioRunA();
    bool PullAudioRunA();
    bool ProcessRunA();
    bool APIRunA();

    bool PullAudioRunB();
    bool PushAudioRunB();
    bool ProcessRunB();
    bool APIRunB();



    
    AudioCodingModule* _acmA;
    AudioCodingModule* _acmB;

    
    Channel* _channel_A2B;
    Channel* _channel_B2A;

    
    
    PCMFile _inFileA;
    PCMFile _outFileA;
    
    PCMFile _outFileB;
    PCMFile _inFileB;

    
    
    WebRtc_Word32 _outFreqHzA;
    
    WebRtc_Word32 _outFreqHzB;

    
    
    
    bool _writeToFile;
    
    
    EventWrapper* _pullEventA;      
    EventWrapper* _pushEventA;      
    EventWrapper* _processEventA;   
    EventWrapper* _apiEventA;       
    
    EventWrapper* _pullEventB;      
    EventWrapper* _pushEventB;      
    EventWrapper* _processEventB;   
    EventWrapper* _apiEventB;       

    
    WebRtc_UWord8 _codecCntrA;
    WebRtc_UWord8 _codecCntrB;

    
    bool _thereIsEncoderA;
    bool _thereIsEncoderB;
    bool _thereIsDecoderA;
    bool _thereIsDecoderB;

    bool             _sendVADA;
    bool             _sendDTXA;
    ACMVADMode       _sendVADModeA;

    bool             _sendVADB;
    bool             _sendDTXB;
    ACMVADMode       _sendVADModeB;

    WebRtc_Word32    _minDelayA;
    WebRtc_Word32    _minDelayB;
    bool             _payloadUsed[32];

    AudioPlayoutMode    _playoutModeA;
    AudioPlayoutMode    _playoutModeB;

    ACMBackgroundNoiseMode _bgnModeA;
    ACMBackgroundNoiseMode _bgnModeB;


    int            _receiveVADActivityA[3];
    int            _receiveVADActivityB[3];
    bool           _verbose;

    int            _dotPositionA;
    int            _dotMoveDirectionA;
    int            _dotPositionB;
    int            _dotMoveDirectionB;

    char           _movingDot[41];

    DTMFDetector*  _dtmfCallback;
    VADCallback*   _vadCallbackA;
    VADCallback*   _vadCallbackB;
    RWLockWrapper&    _apiTestRWLock;
    bool           _randomTest;
    int            _testNumA;
    int            _testNumB;
};

} 

#endif
