












#include <string>
#include <stdio.h>

#include "event_wrapper.h"
#include "udp_transport.h"
#include "rtp_rtcp.h"
#include "trace.h"

#include "TestSenderReceiver.h"
#include "TestLoadGenerator.h"

#include "MatlabPlot.h"



class myTransportCB: public UdpTransportData
{
public:
    myTransportCB (RtpRtcp *rtpMod) : _rtpMod(rtpMod) {};
protected:
    
    virtual void IncomingRTPPacket(const WebRtc_Word8* incomingRtpPacket,
        const WebRtc_Word32 rtpPacketLength,
        const WebRtc_Word8* fromIP,
        const WebRtc_UWord16 fromPort);

    virtual void IncomingRTCPPacket(const WebRtc_Word8* incomingRtcpPacket,
        const WebRtc_Word32 rtcpPacketLength,
        const WebRtc_Word8* fromIP,
        const WebRtc_UWord16 fromPort);

private:
    RtpRtcp *_rtpMod;
};

void myTransportCB::IncomingRTPPacket(const WebRtc_Word8* incomingRtpPacket,
                                      const WebRtc_Word32 rtpPacketLength,
                                      const WebRtc_Word8* fromIP,
                                      const WebRtc_UWord16 fromPort)
{
    printf("Receiving RTP from IP %s, port %u\n", fromIP, fromPort);
    _rtpMod->IncomingPacket((WebRtc_UWord8 *) incomingRtpPacket, static_cast<WebRtc_UWord16>(rtpPacketLength));
}

void myTransportCB::IncomingRTCPPacket(const WebRtc_Word8* incomingRtcpPacket,
                                       const WebRtc_Word32 rtcpPacketLength,
                                       const WebRtc_Word8* fromIP,
                                       const WebRtc_UWord16 fromPort)
{
    printf("Receiving RTCP from IP %s, port %u\n", fromIP, fromPort);
    _rtpMod->IncomingPacket((WebRtc_UWord8 *) incomingRtcpPacket, static_cast<WebRtc_UWord16>(rtcpPacketLength));
}


int main(int argc, char* argv[])
{
    bool isSender = false;
    bool isReceiver = false;
    WebRtc_UWord16 port;
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
    Trace::SetLevelFilter(webrtc::kTraceAll);

    sendrec->InitReceiver(port);

    sendrec->Start();

    if (isSender)
    {
        const WebRtc_UWord32 startRateKbps = 1000;
        
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

