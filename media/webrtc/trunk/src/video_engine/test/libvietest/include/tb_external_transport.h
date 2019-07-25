













#ifndef WEBRTC_VIDEO_ENGINE_TEST_AUTOTEST_INTERFACE_TB_EXTERNAL_TRANSPORT_H_
#define WEBRTC_VIDEO_ENGINE_TEST_AUTOTEST_INTERFACE_TB_EXTERNAL_TRANSPORT_H_

#include <list>

#include "common_types.h"

namespace webrtc
{
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class ViENetwork;
}


class SendFrameCallback
{
public:
    
    
    
    virtual void FrameSent(unsigned int rtp_timestamp) = 0;
protected:
    SendFrameCallback() {}
    virtual ~SendFrameCallback() {}
};



class ReceiveFrameCallback
{
public:
    
    
    
    virtual void FrameReceived(unsigned int rtp_timestamp) = 0;
protected:
    ReceiveFrameCallback() {}
    virtual ~ReceiveFrameCallback() {}
};





class TbExternalTransport : public webrtc::Transport
{
public:
    TbExternalTransport(webrtc::ViENetwork& vieNetwork);
    ~TbExternalTransport(void);

    virtual int SendPacket(int channel, const void *data, int len);
    virtual int SendRTCPPacket(int channel, const void *data, int len);

    
    
    virtual void RegisterSendFrameCallback(SendFrameCallback* callback);

    
    
    virtual void RegisterReceiveFrameCallback(ReceiveFrameCallback* callback);

    
    
    WebRtc_Word32 SetPacketLoss(WebRtc_Word32 lossRate);  
    void SetNetworkDelay(WebRtc_Word64 delayMs);
    void SetSSRCFilter(WebRtc_UWord32 SSRC);

    void ClearStats();
    void GetStats(WebRtc_Word32& numRtpPackets,
                  WebRtc_Word32& numDroppedPackets,
                  WebRtc_Word32& numRtcpPackets);

    void SetTemporalToggle(unsigned char layers);
    void EnableSSRCCheck();
    unsigned int ReceivedSSRC();

    void EnableSequenceNumberCheck();
    unsigned short GetFirstSequenceNumber();

protected:
    static bool ViEExternalTransportRun(void* object);
    bool ViEExternalTransportProcess();
private:
    WebRtc_Word64 NowMs();

    enum
    {
        KMaxPacketSize = 1650
    };
    enum
    {
        KMaxWaitTimeMs = 100
    };
    typedef struct
    {
        WebRtc_Word8 packetBuffer[KMaxPacketSize];
        WebRtc_Word32 length;
        WebRtc_Word32 channel;
        WebRtc_Word64 receiveTime;
    } VideoPacket;

    webrtc::ViENetwork& _vieNetwork;
    webrtc::ThreadWrapper& _thread;
    webrtc::EventWrapper& _event;
    webrtc::CriticalSectionWrapper& _crit;
    webrtc::CriticalSectionWrapper& _statCrit;

    WebRtc_Word32 _lossRate;
    WebRtc_Word64 _networkDelayMs;
    WebRtc_Word32 _rtpCount;
    WebRtc_Word32 _rtcpCount;
    WebRtc_Word32 _dropCount;

    std::list<VideoPacket*> _rtpPackets;
    std::list<VideoPacket*> _rtcpPackets;

    SendFrameCallback* _send_frame_callback;
    ReceiveFrameCallback* _receive_frame_callback;

    unsigned char _temporalLayers;
    unsigned short _seqNum;
    unsigned short _sendPID;
    unsigned char _receivedPID;
    bool _switchLayer;
    unsigned char _currentRelayLayer;
    unsigned int _lastTimeMs;

    bool _checkSSRC;
    WebRtc_UWord32 _lastSSRC;
    bool _filterSSRC;
    WebRtc_UWord32 _SSRC;
    bool _checkSequenceNumber;
    WebRtc_UWord16 _firstSequenceNumber;

    
    
    WebRtc_UWord32 _firstRTPTimestamp;
    
    WebRtc_UWord32 _lastSendRTPTimestamp;
    WebRtc_UWord32 _lastReceiveRTPTimestamp;
};

#endif  
