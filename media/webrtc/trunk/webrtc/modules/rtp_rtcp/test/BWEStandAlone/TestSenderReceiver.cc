









#include <stdio.h>

#include "rtp_rtcp.h"
#include "udp_transport.h"
#include "event_wrapper.h"
#include "thread_wrapper.h"
#include "tick_util.h"
#include "critical_section_wrapper.h"
#include "TestSenderReceiver.h"
#include "TestLoadGenerator.h"
#include <stdlib.h>

#define NR_OF_SOCKET_BUFFERS 500


bool ProcThreadFunction(void *obj)
{
    if (obj == NULL)
    {
        return false;
    }
    TestSenderReceiver *theObj = static_cast<TestSenderReceiver *>(obj);

    return theObj->ProcLoop();
}


TestSenderReceiver::TestSenderReceiver (void)
:
_critSect(CriticalSectionWrapper::CreateCriticalSection()),
_eventPtr(NULL),
_procThread(NULL),
_running(false),
_payloadType(0),
_loadGenerator(NULL),
_isSender(false),
_isReceiver(false),
_timeOut(false),
_sendRecCB(NULL),
_lastBytesReceived(0),
_lastTime(-1)
{
    
    _rtp = RtpRtcp::CreateRtpRtcp(0, false);
    if (!_rtp)
    {
        throw "Could not create RTP/RTCP module";
        exit(1);
    }

    if (_rtp->InitReceiver() != 0)
    {
        throw "_rtp->InitReceiver()";
        exit(1);
    }

    if (_rtp->InitSender() != 0)
    {
        throw "_rtp->InitSender()";
        exit(1);
    }

    
    uint8_t numberOfThreads = 1;
    _transport = UdpTransport::Create(0, numberOfThreads);
    if (!_transport)
    {
        throw "Could not create transport module";
        exit(1);
    }
}

TestSenderReceiver::~TestSenderReceiver (void)
{

    Stop(); 

    _critSect->Enter();

    if (_rtp)
    {
        RtpRtcp::DestroyRtpRtcp(_rtp);
        _rtp = NULL;
    }

    if (_transport)
    {
        UdpTransport::Destroy(_transport);
        _transport = NULL;
    }

    delete _critSect;

}


int32_t TestSenderReceiver::InitReceiver (const uint16_t rtpPort,
                                          const uint16_t rtcpPort,
                                          const int8_t payloadType )
{
    CriticalSectionScoped cs(_critSect);

    
    if (_transport->InitializeReceiveSockets(this, rtpPort) != 0)
    {
        throw "_transport->InitializeReceiveSockets";
        exit(1);
    }

    if (_rtp->RegisterIncomingRTPCallback(this) != 0)
    {
        throw "_rtp->RegisterIncomingRTPCallback";
        exit(1);
    }

    if (_rtp->RegisterIncomingDataCallback(this) != 0)
    {
        throw "_rtp->RegisterIncomingRTPCallback";
        exit(1);
    }

    if (_rtp->SetRTCPStatus(kRtcpNonCompound) != 0)
    {
        throw "_rtp->SetRTCPStatus";
        exit(1);
    }

    if (_rtp->SetTMMBRStatus(true) != 0)
    {
        throw "_rtp->SetTMMBRStatus";
        exit(1);
    }

    if (_rtp->RegisterReceivePayload("I420", payloadType, 90000) != 0)
    {
        throw "_rtp->RegisterReceivePayload";
        exit(1);
    }

    _isReceiver = true;

    return (0);
}


int32_t TestSenderReceiver::Start()
{
    CriticalSectionScoped cs(_critSect);

    _eventPtr = EventWrapper::Create();

    if (_rtp->SetSendingStatus(true) != 0)
    {
        throw "_rtp->SetSendingStatus";
        exit(1);
    }

    _procThread = ThreadWrapper::CreateThread(ProcThreadFunction, this, kRealtimePriority, "TestSenderReceiver");
    if (_procThread == NULL)
    {
        throw "Unable to create process thread";
        exit(1);
    }

    _running = true;

    if (_isReceiver)
    {
        if (_transport->StartReceiving(NR_OF_SOCKET_BUFFERS) != 0)
        {
            throw "_transport->StartReceiving";
            exit(1);
        }
    }

    unsigned int tid;
    _procThread->Start(tid);

    return 0;

}


int32_t TestSenderReceiver::Stop ()
{
    CriticalSectionScoped cs(_critSect);

    _transport->StopReceiving();

    if (_procThread)
    {
        _procThread->SetNotAlive();
        _running = false;
        _eventPtr->Set();

        while (!_procThread->Stop())
        {
            ;
        }

        delete _eventPtr;

        delete _procThread;
    }

    _procThread = NULL;

    return (0);
}


bool TestSenderReceiver::ProcLoop(void)
{

    
    _rtp->Process();

    
    _transport->Process();

    
    while (_running)
    {
        
        int32_t rtpWait = _rtp->TimeUntilNextProcess();

        
        int32_t tpWait = _transport->TimeUntilNextProcess();

        int32_t minWait = (rtpWait < tpWait) ? rtpWait: tpWait;
        minWait = (minWait > 0) ? minWait : 0;
        
        _eventPtr->Wait(minWait);

        
        _rtp->Process();

        
        _transport->Process();

    }

    return true;
}


int32_t TestSenderReceiver::ReceiveBitrateKbps ()
{
    uint32_t bytesSent;
    uint32_t packetsSent;
    uint32_t bytesReceived;
    uint32_t packetsReceived;

    if (_rtp->DataCountersRTP(&bytesSent, &packetsSent, &bytesReceived, &packetsReceived) == 0)
    {
        int64_t now = TickTime::MillisecondTimestamp();
        int32_t kbps = 0;
        if (now > _lastTime)
        {
            if (_lastTime > 0)
            {
                
                kbps = static_cast<int32_t>(
                    (8 * (bytesReceived - _lastBytesReceived)) / (now - _lastTime));
            }
            _lastTime = now;
            _lastBytesReceived = bytesReceived;
        }
        return (kbps);
    }

    return (-1);
}


int32_t TestSenderReceiver::SetPacketTimeout(const uint32_t timeoutMS)
{
    return (_rtp->SetPacketTimeout(timeoutMS, 0 ));
}


void TestSenderReceiver::OnPacketTimeout(const int32_t id)
{
    CriticalSectionScoped lock(_critSect);

    _timeOut = true;
}


void TestSenderReceiver::OnReceivedPacket(const int32_t id,
                                    const RtpRtcpPacketType packetType)
{
    
    

}

int32_t TestSenderReceiver::OnReceivedPayloadData(const uint8_t* payloadData,
                                                  const uint16_t payloadSize,
                                                  const webrtc::WebRtcRTPHeader* rtpHeader)
{
    
    return (0);
}


void TestSenderReceiver::IncomingRTPPacket(const int8_t* incomingRtpPacket,
                                      const int32_t rtpPacketLength,
                                      const int8_t* fromIP,
                                      const uint16_t fromPort)
{
    _rtp->IncomingPacket((uint8_t *) incomingRtpPacket, static_cast<uint16_t>(rtpPacketLength));
}



void TestSenderReceiver::IncomingRTCPPacket(const int8_t* incomingRtcpPacket,
                                       const int32_t rtcpPacketLength,
                                       const int8_t* fromIP,
                                       const uint16_t fromPort)
{
    _rtp->IncomingPacket((uint8_t *) incomingRtcpPacket, static_cast<uint16_t>(rtcpPacketLength));
}








int32_t TestSenderReceiver::InitSender (const uint32_t startBitrateKbps,
                                        const int8_t* ipAddr,
                                        const uint16_t rtpPort,
                                        const uint16_t rtcpPort ,
                                        const int8_t payloadType )
{
    CriticalSectionScoped cs(_critSect);

    _payloadType = payloadType;

    
    if (_loadGenerator)
    {
        _loadGenerator->SetBitrate(startBitrateKbps);
    }

    if (_rtp->RegisterSendTransport(_transport) != 0)
    {
        throw "_rtp->RegisterSendTransport";
        exit(1);
    }
    if (_rtp->RegisterSendPayload("I420", _payloadType, 90000) != 0)
    {
        throw "_rtp->RegisterSendPayload";
        exit(1);
    }

    if (_rtp->RegisterIncomingVideoCallback(this) != 0)
    {
        throw "_rtp->RegisterIncomingVideoCallback";
        exit(1);
    }

    if (_rtp->SetRTCPStatus(kRtcpNonCompound) != 0)
    {
        throw "_rtp->SetRTCPStatus";
        exit(1);
    }

    if (_rtp->SetSendBitrate(startBitrateKbps*1000, 0, MAX_BITRATE_KBPS) != 0)
    {
        throw "_rtp->SetSendBitrate";
        exit(1);
    }


    
    if (_transport->InitializeSendSockets(ipAddr, rtpPort, rtcpPort))
    {
        throw "_transport->InitializeSendSockets";
        exit(1);
    }

    _isSender = true;

    return (0);
}



int32_t
TestSenderReceiver::SendOutgoingData(const uint32_t timeStamp,
                                     const uint8_t* payloadData,
                                     const uint32_t payloadSize,
                                     const webrtc::FrameType frameType )
{
    return (_rtp->SendOutgoingData(frameType, _payloadType, timeStamp, payloadData, payloadSize));
}


int32_t TestSenderReceiver::SetLoadGenerator(TestLoadGenerator *generator)
{
    CriticalSectionScoped cs(_critSect);

    _loadGenerator = generator;
    return(0);

}

void TestSenderReceiver::OnNetworkChanged(const int32_t id,
                                  const uint32_t minBitrateBps,
                                  const uint32_t maxBitrateBps,
                                  const uint8_t fractionLost,
                                  const uint16_t roundTripTimeMs,
                                  const uint16_t bwEstimateKbitMin,
                                  const uint16_t bwEstimateKbitMax)
{
    if (_loadGenerator)
    {
        _loadGenerator->SetBitrate(maxBitrateBps/1000);
    }

    if (_sendRecCB)
    {
        _sendRecCB->OnOnNetworkChanged(maxBitrateBps,
            fractionLost,
            roundTripTimeMs,
            bwEstimateKbitMin,
            bwEstimateKbitMax);
    }
}
