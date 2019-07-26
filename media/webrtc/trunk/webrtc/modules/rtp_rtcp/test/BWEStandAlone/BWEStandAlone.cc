












#include <stdio.h>
#include <string>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/channel_transport/udp_transport.h"

#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/TestLoadGenerator.h"
#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/TestSenderReceiver.h"

#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/MatlabPlot.h"



class myTransportCB: public UdpTransportData
{
public:
    myTransportCB (RtpRtcp *rtpMod) : _rtpMod(rtpMod) {};
protected:
    
    virtual void IncomingRTPPacket(const int8_t* incomingRtpPacket,
        const int32_t rtpPacketLength,
        const int8_t* fromIP,
        const uint16_t fromPort);

    virtual void IncomingRTCPPacket(const int8_t* incomingRtcpPacket,
        const int32_t rtcpPacketLength,
        const int8_t* fromIP,
        const uint16_t fromPort);

private:
    RtpRtcp *_rtpMod;
};

void myTransportCB::IncomingRTPPacket(const int8_t* incomingRtpPacket,
                                      const int32_t rtpPacketLength,
                                      const int8_t* fromIP,
                                      const uint16_t fromPort)
{
    printf("Receiving RTP from IP %s, port %u\n", fromIP, fromPort);
    _rtpMod->IncomingPacket((uint8_t *) incomingRtpPacket, static_cast<uint16_t>(rtpPacketLength));
}

void myTransportCB::IncomingRTCPPacket(const int8_t* incomingRtcpPacket,
                                       const int32_t rtcpPacketLength,
                                       const int8_t* fromIP,
                                       const uint16_t fromPort)
{
    printf("Receiving RTCP from IP %s, port %u\n", fromIP, fromPort);
    _rtpMod->IncomingPacket((uint8_t *) incomingRtcpPacket, static_cast<uint16_t>(rtcpPacketLength));
}


int main(int argc, char* argv[])
{
    bool isSender = false;
    bool isReceiver = false;
    uint16_t port;
    std::string ip;
    TestSenderReceiver *sendrec = new TestSenderReceiver();
    TestLoadGenerator *gen;

    if (argc == 2)
    {
        
        isReceiver = true;

        
        port = atoi(argv[1]);
    }
    else if (argc == 3)
    {
        
        isSender = true;
        isReceiver = true;

        
        ip = argv[1];

        
        port = atoi(argv[2]);
    }

    Trace::CreateTrace();
    Trace::SetTraceFile("BWEStandAloneTrace.txt");
    Trace::set_level_filter(webrtc::kTraceAll);

    sendrec->InitReceiver(port);

    sendrec->Start();

    if (isSender)
    {
        const uint32_t startRateKbps = 1000;
        
        gen = new CBRFixFRGenerator(sendrec, startRateKbps, 90000, 30, 0.2);
        
        
        
        
        
        sendrec->SetLoadGenerator(gen);
        sendrec->InitSender(startRateKbps, ip.c_str(), port);
        gen->Start();
    }

    while (1)
    {
    }

    if (isSender)
    {
        gen->Stop();
        delete gen;
    }

    delete sendrec;

    
    

    
    
    
    
    
    
    
    
    


    
    


    return(0);
 
 
 

 
 

 
 
 
 



 
 
 
 
 
 
 
 


    
    
    
    
    
 
    
 

 
 
 
 
 

 
 
 
 
 


    
}
