













#ifndef WEBRTC_VIDEO_ENGINE_TEST_AUTOTEST_INTERFACE_TB_EXTERNAL_TRANSPORT_H_
#define WEBRTC_VIDEO_ENGINE_TEST_AUTOTEST_INTERFACE_TB_EXTERNAL_TRANSPORT_H_

#include <list>
#include <map>

#include "webrtc/common_types.h"

namespace webrtc
{
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class ViENetwork;
}

enum RandomLossModel {
  kNoLoss,
  kUniformLoss,
  kGilbertElliotLoss
};
struct NetworkParameters {
  int packet_loss_rate;
  int burst_length;  
  int mean_one_way_delay;
  int std_dev_one_way_delay;
  RandomLossModel loss_model;
  NetworkParameters():
    packet_loss_rate(0), burst_length(0), mean_one_way_delay(0),
        std_dev_one_way_delay(0), loss_model(kNoLoss) {}
};


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
    typedef std::map<unsigned int, unsigned int> SsrcChannelMap;

    TbExternalTransport(webrtc::ViENetwork& vieNetwork,
                        int sender_channel,
                        TbExternalTransport::SsrcChannelMap* receive_channels);
    ~TbExternalTransport(void);

    virtual int SendPacket(int channel, const void *data, int len) OVERRIDE;
    virtual int SendRTCPPacket(int channel, const void *data, int len) OVERRIDE;

    
    
    virtual void RegisterSendFrameCallback(SendFrameCallback* callback);

    
    
    virtual void RegisterReceiveFrameCallback(ReceiveFrameCallback* callback);

    
    
    void SetNetworkParameters(const NetworkParameters& network_parameters);
    void SetSSRCFilter(uint32_t SSRC);

    void ClearStats();
    
    
    void GetStats(int32_t& numRtpPackets,
                  int32_t& numDroppedPackets,
                  int32_t& numRtcpPackets,
                  std::map<uint8_t, int>* packet_counters);

    void SetTemporalToggle(unsigned char layers);
    void EnableSSRCCheck();
    unsigned int ReceivedSSRC();

    void EnableSequenceNumberCheck();
    unsigned short GetFirstSequenceNumber();

    bool EmptyQueue() const;

protected:
    static bool ViEExternalTransportRun(void* object);
    bool ViEExternalTransportProcess();
private:
    
    static int GaussianRandom(int mean_ms, int standard_deviation_ms);
    bool UniformLoss(int loss_rate);
    bool GilbertElliotLoss(int loss_rate, int burst_length);
    int64_t NowMs();

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
        int8_t packetBuffer[KMaxPacketSize];
        int32_t length;
        int32_t channel;
        int64_t receiveTime;
    } VideoPacket;

    int sender_channel_;
    SsrcChannelMap* receive_channels_;
    webrtc::ViENetwork& _vieNetwork;
    webrtc::ThreadWrapper& _thread;
    webrtc::EventWrapper& _event;
    webrtc::CriticalSectionWrapper& _crit;
    webrtc::CriticalSectionWrapper& _statCrit;

    NetworkParameters network_parameters_;
    int32_t _rtpCount;
    int32_t _rtcpCount;
    int32_t _dropCount;
    
    
    std::map<uint8_t, int> packet_counters_;

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
    uint32_t _lastSSRC;
    bool _filterSSRC;
    uint32_t _SSRC;
    bool _checkSequenceNumber;
    uint16_t _firstSequenceNumber;

    
    
    uint32_t _firstRTPTimestamp;
    
    uint32_t _lastSendRTPTimestamp;
    uint32_t _lastReceiveRTPTimestamp;
    int64_t last_receive_time_;
    bool previous_drop_;
};

#endif  
