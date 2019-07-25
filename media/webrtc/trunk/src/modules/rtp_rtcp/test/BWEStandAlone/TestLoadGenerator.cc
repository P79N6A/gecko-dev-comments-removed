









#include <algorithm> 
#include <stdio.h>

#include "TestLoadGenerator.h"
#include "TestSenderReceiver.h"
#include "event_wrapper.h"
#include "thread_wrapper.h"
#include "critical_section_wrapper.h"
#include "tick_util.h"


bool SenderThreadFunction(void *obj)
{
    if (obj == NULL)
    {
        return false;
    }
    TestLoadGenerator *_genObj = static_cast<TestLoadGenerator *>(obj);

    return _genObj->GeneratorLoop();
}


TestLoadGenerator::TestLoadGenerator(TestSenderReceiver *sender, WebRtc_Word32 rtpSampleRate)
:
_critSect(CriticalSectionWrapper::CreateCriticalSection()),
_eventPtr(NULL),
_genThread(NULL),
_bitrateKbps(0),
_sender(sender),
_running(false),
_rtpSampleRate(rtpSampleRate)
{
}

TestLoadGenerator::~TestLoadGenerator ()
{
    if (_running)
    {
        Stop();
    }

    delete _critSect;
}

WebRtc_Word32 TestLoadGenerator::SetBitrate (WebRtc_Word32 newBitrateKbps)
{
    CriticalSectionScoped cs(_critSect);

    if (newBitrateKbps < 0)
    {
        return -1;
    }

    _bitrateKbps = newBitrateKbps;

    printf("New bitrate = %i kbps\n", _bitrateKbps);

    return _bitrateKbps;
}


WebRtc_Word32 TestLoadGenerator::Start (const char *threadName)
{
    CriticalSectionScoped cs(_critSect);

    _eventPtr = EventWrapper::Create();

    _genThread = ThreadWrapper::CreateThread(SenderThreadFunction, this, kRealtimePriority, threadName);
    if (_genThread == NULL)
    {
        throw "Unable to start generator thread";
        exit(1);
    }

    _running = true;

    unsigned int tid;
    _genThread->Start(tid);

    return 0;
}


WebRtc_Word32 TestLoadGenerator::Stop ()
{
    _critSect.Enter();

    if (_genThread)
    {
        _genThread->SetNotAlive();
        _running = false;
        _eventPtr->Set();

        while (!_genThread->Stop())
        {
            _critSect.Leave();
            _critSect.Enter();
        }

        delete _genThread;
        _genThread = NULL;

        delete _eventPtr;
        _eventPtr = NULL;
    }

    _genThread = NULL;
    _critSect.Leave();
    return (0);
}


int TestLoadGenerator::generatePayload ()
{
    return(generatePayload( static_cast<WebRtc_UWord32>( TickTime::MillisecondTimestamp() * _rtpSampleRate / 1000 )));
}


int TestLoadGenerator::sendPayload (const WebRtc_UWord32 timeStamp,
                                    const WebRtc_UWord8* payloadData,
                                    const WebRtc_UWord32 payloadSize,
                                    const webrtc::FrameType frameType )
{

    return (_sender->SendOutgoingData(timeStamp, payloadData, payloadSize, frameType));
}


CBRGenerator::CBRGenerator (TestSenderReceiver *sender, WebRtc_Word32 payloadSizeBytes, WebRtc_Word32 bitrateKbps, WebRtc_Word32 rtpSampleRate)
:

_payloadSizeBytes(payloadSizeBytes),
_payload(new WebRtc_UWord8[payloadSizeBytes]),
TestLoadGenerator(sender, rtpSampleRate)
{
    SetBitrate (bitrateKbps);
}

CBRGenerator::~CBRGenerator ()
{
    if (_running)
    {
        Stop();
    }

    if (_payload)
    {
        delete [] _payload;
    }

}

bool CBRGenerator::GeneratorLoop ()
{
    double periodMs;
    WebRtc_Word64 nextSendTime = TickTime::MillisecondTimestamp();


    
    while (_running)
    {
        
        generatePayload( static_cast<WebRtc_UWord32>(nextSendTime * _rtpSampleRate / 1000) );

        
        periodMs = 8.0 * _payloadSizeBytes / ( _bitrateKbps );

        nextSendTime = static_cast<WebRtc_Word64>(nextSendTime + periodMs);

        WebRtc_Word32 waitTime = static_cast<WebRtc_Word32>(nextSendTime - TickTime::MillisecondTimestamp());
        if (waitTime < 0)
        {
            waitTime = 0;
        }
        
        _eventPtr->Wait(static_cast<WebRtc_Word32>(waitTime));
    }

    return true;
}

int CBRGenerator::generatePayload ( WebRtc_UWord32 timestamp )
{
    CriticalSectionScoped cs(_critSect);

    

    int ret = sendPayload(timestamp, _payload, _payloadSizeBytes);

    
    return ret;
}






CBRFixFRGenerator::CBRFixFRGenerator (TestSenderReceiver *sender, WebRtc_Word32 bitrateKbps,
                                      WebRtc_Word32 rtpSampleRate, WebRtc_Word32 frameRateFps ,
                                      double spread )
:

_payloadSizeBytes(0),
_payload(NULL),
_payloadAllocLen(0),
_frameRateFps(frameRateFps),
_spreadFactor(spread),
TestLoadGenerator(sender, rtpSampleRate)
{
    SetBitrate (bitrateKbps);
}

CBRFixFRGenerator::~CBRFixFRGenerator ()
{
    if (_running)
    {
        Stop();
    }

    if (_payload)
    {
        delete [] _payload;
        _payloadAllocLen = 0;
    }

}

bool CBRFixFRGenerator::GeneratorLoop ()
{
    double periodMs;
    WebRtc_Word64 nextSendTime = TickTime::MillisecondTimestamp();

    _critSect.Enter();

    if (_frameRateFps <= 0)
    {
        return false;
    }

    _critSect.Leave();

    
    while (_running)
    {
        _critSect.Enter();

        
        _payloadSizeBytes = nextPayloadSize();

        if (_payloadSizeBytes > 0)
        {

            if (_payloadAllocLen < _payloadSizeBytes * (1 + _spreadFactor))
            {
                
                if (_payload)
                {
                    delete [] _payload;
                    _payload = NULL;
                }

                _payloadAllocLen = static_cast<WebRtc_Word32>((_payloadSizeBytes * (1 + _spreadFactor) * 3) / 2 + .5); 
                _payload = new WebRtc_UWord8[_payloadAllocLen];
            }


            
            generatePayload( static_cast<WebRtc_UWord32>(nextSendTime * _rtpSampleRate / 1000) );
        }

        _critSect.Leave();

        
        periodMs = 1000.0 / _frameRateFps;
        nextSendTime = static_cast<WebRtc_Word64>(nextSendTime + periodMs + 0.5);

        WebRtc_Word32 waitTime = static_cast<WebRtc_Word32>(nextSendTime - TickTime::MillisecondTimestamp());
        if (waitTime < 0)
        {
            waitTime = 0;
        }
        
        _eventPtr->Wait(waitTime);
    }

    return true;
}

WebRtc_Word32 CBRFixFRGenerator::nextPayloadSize()
{
    const double periodMs = 1000.0 / _frameRateFps;
    return static_cast<WebRtc_Word32>(_bitrateKbps * periodMs / 8 + 0.5);
}

int CBRFixFRGenerator::generatePayload ( WebRtc_UWord32 timestamp )
{
    CriticalSectionScoped cs(_critSect);

    double factor = ((double) rand() - RAND_MAX/2) / RAND_MAX; 
    factor = 1 + 2 * _spreadFactor * factor; 

    WebRtc_Word32 thisPayloadBytes = static_cast<WebRtc_Word32>(_payloadSizeBytes * factor);
    
    if (thisPayloadBytes > _payloadAllocLen)
    {
        thisPayloadBytes = _payloadAllocLen;
    }

    int ret = sendPayload(timestamp, _payload, thisPayloadBytes);
    return ret;
}




PeriodicKeyFixFRGenerator::PeriodicKeyFixFRGenerator (TestSenderReceiver *sender, WebRtc_Word32 bitrateKbps,
                                                      WebRtc_Word32 rtpSampleRate, WebRtc_Word32 frameRateFps ,
                                                      double spread , double keyFactor , WebRtc_UWord32 keyPeriod )
:
_keyFactor(keyFactor),
_keyPeriod(keyPeriod),
_frameCount(0),
CBRFixFRGenerator(sender, bitrateKbps, rtpSampleRate, frameRateFps, spread)
{
}

WebRtc_Word32 PeriodicKeyFixFRGenerator::nextPayloadSize()
{
    
    WebRtc_Word32 payloadSizeBytes = static_cast<WebRtc_Word32>(1000 * _bitrateKbps / (8.0 * _frameRateFps * (1.0 + (_keyFactor - 1.0) / _keyPeriod)) + 0.5);

    if (_frameCount % _keyPeriod == 0)
    {
        
        payloadSizeBytes = static_cast<WebRtc_Word32>(_keyFactor * _payloadSizeBytes + 0.5);
    }
    _frameCount++;

    return payloadSizeBytes;
}



CBRVarFRGenerator::CBRVarFRGenerator(TestSenderReceiver *sender, WebRtc_Word32 bitrateKbps, const WebRtc_UWord8* frameRates,
                                     WebRtc_UWord16 numFrameRates, WebRtc_Word32 rtpSampleRate, double avgFrPeriodMs,
                                     double frSpreadFactor, double spreadFactor)
:
_avgFrPeriodMs(avgFrPeriodMs),
_frSpreadFactor(frSpreadFactor),
_frameRates(NULL),
_numFrameRates(numFrameRates),
_frChangeTimeMs(TickTime::MillisecondTimestamp() + _avgFrPeriodMs),
CBRFixFRGenerator(sender, bitrateKbps, rtpSampleRate, frameRates[0], spreadFactor)
{
    _frameRates = new WebRtc_UWord8[_numFrameRates];
    memcpy(_frameRates, frameRates, _numFrameRates);
}

CBRVarFRGenerator::~CBRVarFRGenerator()
{
    delete [] _frameRates;
}

void CBRVarFRGenerator::ChangeFrameRate()
{
    const WebRtc_Word64 nowMs = TickTime::MillisecondTimestamp();
    if (nowMs < _frChangeTimeMs)
    {
        return;
    }
    
    WebRtc_UWord16 frIndex = static_cast<WebRtc_UWord16>(static_cast<double>(rand()) / RAND_MAX
                                            * (_numFrameRates - 1) + 0.5) ;
    assert(frIndex < _numFrameRates);
    _frameRateFps = _frameRates[frIndex];
    
    double factor = ((double) rand() - RAND_MAX/2) / RAND_MAX; 
    factor = 1 + 2 * _frSpreadFactor * factor; 
    _frChangeTimeMs = nowMs + static_cast<WebRtc_Word64>(1000.0 * factor *
                                    _avgFrPeriodMs + 0.5);

    printf("New frame rate: %d\n", _frameRateFps);
}

WebRtc_Word32 CBRVarFRGenerator::nextPayloadSize()
{
    ChangeFrameRate();
    return CBRFixFRGenerator::nextPayloadSize();
}



CBRFrameDropGenerator::CBRFrameDropGenerator(TestSenderReceiver *sender, WebRtc_Word32 bitrateKbps,
                                         WebRtc_Word32 rtpSampleRate, double spreadFactor)
:
_accBits(0),
CBRFixFRGenerator(sender, bitrateKbps, rtpSampleRate, 30, spreadFactor)
{
}

CBRFrameDropGenerator::~CBRFrameDropGenerator()
{
}

WebRtc_Word32 CBRFrameDropGenerator::nextPayloadSize()
{
    _accBits -= 1000 * _bitrateKbps / _frameRateFps;
    if (_accBits < 0)
    {
        _accBits = 0;
    }
    if (_accBits > 0.3 * _bitrateKbps * 1000)
    {
        
        return 0;
    }
    else
    {
        
        const double periodMs = 1000.0 / _frameRateFps;
        WebRtc_Word32 frameSize = static_cast<WebRtc_Word32>(_bitrateKbps * periodMs / 8 + 0.5);
        frameSize = std::max(frameSize, static_cast<WebRtc_Word32>(300 * periodMs / 8 + 0.5));
        _accBits += frameSize * 8;
        return frameSize;
    }
}
