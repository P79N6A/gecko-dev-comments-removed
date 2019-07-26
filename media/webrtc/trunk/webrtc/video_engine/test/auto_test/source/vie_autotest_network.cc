













#include "vie_autotest_defines.h"
#include "vie_autotest.h"
#include "engine_configurations.h"

#include "tb_capture_device.h"
#include "tb_external_transport.h"
#include "tb_interfaces.h"
#include "tb_video_channel.h"

#if defined(_WIN32)
#include <qos.h>
#endif

class ViEAutoTestNetworkObserver: public webrtc::ViENetworkObserver
{
public:
    ViEAutoTestNetworkObserver()
    {
    }
    virtual ~ViEAutoTestNetworkObserver()
    {
    }
    virtual void OnPeriodicDeadOrAlive(const int videoChannel, const bool alive)
    {
    }
    virtual void PacketTimeout(const int videoChannel,
                               const webrtc::ViEPacketTimeout timeout)
    {
    }
};

void ViEAutoTest::ViENetworkStandardTest()
{
    TbInterfaces ViE("ViENetworkStandardTest"); 
    TbCaptureDevice tbCapture(ViE);
    {
        
        TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
        tbCapture.ConnectTo(tbChannel.videoChannel);

        RenderCaptureDeviceAndOutputStream(&ViE, &tbChannel, &tbCapture);

        
        
        

        
        
        
        TbExternalTransport testTransport(*ViE.network, tbChannel.videoChannel,
                                          NULL);
        EXPECT_EQ(0, ViE.network->RegisterSendTransport(
            tbChannel.videoChannel, testTransport));
        EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.rtp_rtcp->SetKeyFrameRequestMethod(
            tbChannel.videoChannel, webrtc::kViEKeyFrameRequestPliRtcp));

        ViETest::Log("Call started using external transport, video should "
            "see video in both windows\n");
        AutoTestSleep(kAutoTestSleepTimeMs);

        EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.network->DeregisterSendTransport(
            tbChannel.videoChannel));

        char myIpAddress[64];
        memset(myIpAddress, 0, 64);
        unsigned short rtpPort = 1234;
        memcpy(myIpAddress, "127.0.0.1", sizeof("127.0.0.1"));
        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, rtpPort, rtpPort + 1, myIpAddress));
        EXPECT_EQ(0, ViE.network->SetSendDestination(
            tbChannel.videoChannel, myIpAddress, rtpPort,
            rtpPort + 1, rtpPort));
        EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

        ViETest::Log("Changed to WebRTC SocketTransport, you should still see "
                     "video in both windows\n");
        AutoTestSleep(kAutoTestSleepTimeMs);

        EXPECT_EQ(0, ViE.network->SetSourceFilter(
            tbChannel.videoChannel, rtpPort + 10, rtpPort + 11, myIpAddress));
        ViETest::Log("Added UDP port filter for incorrect ports, you should "
                     "not see video in Window2");
        AutoTestSleep(2000);
        EXPECT_EQ(0, ViE.network->SetSourceFilter(
            tbChannel.videoChannel, rtpPort, rtpPort + 1, "123.1.1.0"));
        ViETest::Log("Added IP filter for incorrect IP address, you should not "
                     "see video in Window2");
        AutoTestSleep(2000);
        EXPECT_EQ(0, ViE.network->SetSourceFilter(
            tbChannel.videoChannel, rtpPort, rtpPort + 1, myIpAddress));
        ViETest::Log("Added IP filter for this computer, you should see video "
                     "in Window2 again\n");
        AutoTestSleep(kAutoTestSleepTimeMs);

        tbCapture.Disconnect(tbChannel.videoChannel);
    }
}

void ViEAutoTest::ViENetworkExtendedTest()
{
    
    
    

    TbInterfaces ViE("ViENetworkExtendedTest"); 
    TbCaptureDevice tbCapture(ViE);
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));

    {
        
        
        
        
        TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
        tbCapture.ConnectTo(tbChannel.videoChannel);
        const char* remoteIp = "192.168.200.1";
        int DSCP = 0;
        bool useSetSockOpt = false;

        webrtc::VideoCodec videoCodec;
        EXPECT_EQ(0, ViE.codec->GetSendCodec(
            tbChannel.videoChannel, videoCodec));
        videoCodec.maxFramerate = 5;
        EXPECT_EQ(0, ViE.codec->SetSendCodec(
            tbChannel.videoChannel, videoCodec));

        
        
        

        char myIpAddress[64];
        memset(myIpAddress, 0, 64);
        unsigned short rtpPort = 9000;
        EXPECT_EQ(0, ViE.network->GetLocalIP(myIpAddress, false));
        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, rtpPort, rtpPort + 1, myIpAddress));
        EXPECT_EQ(0, ViE.network->SetSendDestination(
            tbChannel.videoChannel, remoteIp, rtpPort, rtpPort + 1, rtpPort));

        
        int tos_result = ViE.network->SetSendToS(tbChannel.videoChannel, 2);
        EXPECT_EQ(0, tos_result);
        if (tos_result != 0)
        {
            ViETest::Log("ViESetSendToS error!.");
            ViETest::Log("You must be admin to run these tests.");
            ViETest::Log("On Win7 and late Vista, you need to right click the "
                         "exe and choose");
            ViETest::Log("\"Run as administrator\"\n");
            getchar();
        }
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));  

        EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

        ViETest::Log("Use Wireshark to capture the outgoing video stream and "
                     "verify ToS settings\n");
        ViETest::Log(" DSCP set to 0x%x\n", DSCP);
        AutoTestSleep(1000);

        EXPECT_EQ(0, ViE.network->SetSendToS(tbChannel.videoChannel, 63));
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));  
        ViETest::Log(" DSCP set to 0x%x\n", DSCP);
        AutoTestSleep(1000);

        EXPECT_EQ(0, ViE.network->SetSendToS(tbChannel.videoChannel, 0));
        EXPECT_EQ(0, ViE.network->SetSendToS(tbChannel.videoChannel, 2, true));
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));  
        ViETest::Log(" DSCP set to 0x%x\n", DSCP);
        AutoTestSleep(1000);

        EXPECT_EQ(0, ViE.network->SetSendToS(tbChannel.videoChannel, 63, true));
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));  
        ViETest::Log(" DSCP set to 0x%x\n", DSCP);
        AutoTestSleep(1000);

        tbCapture.Disconnect(tbChannel.videoChannel);
    }

    
    
    
}

void ViEAutoTest::ViENetworkAPITest()
{
    
    
    

    TbInterfaces ViE("ViENetworkAPITest"); 
    {
        
        TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecI420);

        
        
        

        
        
        
        TbExternalTransport testTransport(*ViE.network, tbChannel.videoChannel,
                                          NULL);
        EXPECT_EQ(0, ViE.network->RegisterSendTransport(
            tbChannel.videoChannel, testTransport));
        EXPECT_NE(0, ViE.network->RegisterSendTransport(
            tbChannel.videoChannel, testTransport));

        
        unsigned char packet[3000];
        memset(packet, 0, sizeof(packet));
        packet[0] = 0x80; 
        packet[1] = 0x7C; 

        
        unsigned char rtcpacket[3000];
        memset(rtcpacket,0, sizeof(rtcpacket));
        rtcpacket[0] = 0x80; 
        rtcpacket[1] = 0xCC; 
        rtcpacket[2] = 0x0;
        rtcpacket[3] = 0x03; 

        EXPECT_NE(0, ViE.network->ReceivedRTPPacket(
            tbChannel.videoChannel, packet, 1500));
        EXPECT_NE(0, ViE.network->ReceivedRTCPPacket(
            tbChannel.videoChannel, rtcpacket, 1500));
        EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.network->ReceivedRTPPacket(
            tbChannel.videoChannel, packet, 1500));
        EXPECT_EQ(0, ViE.network->ReceivedRTCPPacket(
            tbChannel.videoChannel, rtcpacket, 1500));
        EXPECT_NE(0, ViE.network->ReceivedRTPPacket(
            tbChannel.videoChannel, packet, 11));
        EXPECT_NE(0, ViE.network->ReceivedRTPPacket(
            tbChannel.videoChannel, packet, 11));
        EXPECT_EQ(0, ViE.network->ReceivedRTPPacket(
            tbChannel.videoChannel, packet, 3000));
        EXPECT_EQ(0, ViE.network->ReceivedRTPPacket(
            tbChannel.videoChannel, packet, 3000));
        EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));
        EXPECT_NE(0, ViE.network->DeregisterSendTransport(
            tbChannel.videoChannel));  
        EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.network->DeregisterSendTransport(
            tbChannel.videoChannel));
        EXPECT_NE(0, ViE.network->DeregisterSendTransport(
            tbChannel.videoChannel));  

        
        
        
        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, 1234, 1235, "127.0.0.1"));
        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, 1234, 1235, "127.0.0.1"));
        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, 1236, 1237, "127.0.0.1"));

        unsigned short rtpPort = 0;
        unsigned short rtcpPort = 0;
        char ipAddress[64];
        memset(ipAddress, 0, 64);
        EXPECT_EQ(0, ViE.network->GetLocalReceiver(
            tbChannel.videoChannel, rtpPort, rtcpPort, ipAddress));
        EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
        EXPECT_NE(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, 1234, 1235, "127.0.0.1"));
        EXPECT_EQ(0, ViE.network->GetLocalReceiver(
            tbChannel.videoChannel, rtpPort, rtcpPort, ipAddress));
        EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));

        
        
        
        EXPECT_EQ(0, ViE.network->SetSendDestination(
            tbChannel.videoChannel, "127.0.0.1", 1234, 1235, 1234, 1235));
        EXPECT_EQ(0, ViE.network->SetSendDestination(
            tbChannel.videoChannel, "127.0.0.1", 1236, 1237, 1234, 1235));

        unsigned short sourceRtpPort = 0;
        unsigned short sourceRtcpPort = 0;
        EXPECT_EQ(0, ViE.network->GetSendDestination(
            tbChannel.videoChannel, ipAddress, rtpPort, rtcpPort,
            sourceRtpPort, sourceRtcpPort));
        EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

        
        EXPECT_NE(0, ViE.network->SetSendDestination(
            tbChannel.videoChannel, "127.0.0.1", 1234, 1235, 1234, 1235));
        EXPECT_EQ(kViENetworkAlreadySending, ViE.base->LastError());

        EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.network->SetSendDestination(
            tbChannel.videoChannel, "127.0.0.1", 1234, 1235, 1234, 1235));
        EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));
        EXPECT_EQ(0, ViE.network->GetSendDestination(
            tbChannel.videoChannel, ipAddress, rtpPort, rtcpPort,
            sourceRtpPort, sourceRtcpPort));
        EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

        
        
        

        
        EXPECT_EQ(0, ViE.network->GetLocalIP(ipAddress, false));

        

        
        
        
        EXPECT_NE(0, ViE.network->GetSourceFilter(
            tbChannel.videoChannel, rtpPort, rtcpPort, ipAddress));
        EXPECT_EQ(0, ViE.network->SetSourceFilter(
            tbChannel.videoChannel, 1234, 1235, "10.10.10.10"));
        EXPECT_EQ(0, ViE.network->SetSourceFilter(
            tbChannel.videoChannel, 1236, 1237, "127.0.0.1"));
        EXPECT_EQ(0, ViE.network->GetSourceFilter(
            tbChannel.videoChannel, rtpPort, rtcpPort, ipAddress));
        EXPECT_EQ(0, ViE.network->SetSourceFilter(
            tbChannel.videoChannel, 0, 0, NULL));
        EXPECT_NE(0, ViE.network->GetSourceFilter(
            tbChannel.videoChannel, rtpPort, rtcpPort, ipAddress));
    }
    {
        TbVideoChannel tbChannel(ViE);  
        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, 1234));

        int DSCP = 0;
        bool useSetSockOpt = false;
        
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));  
        EXPECT_EQ(0, DSCP);

        
        EXPECT_NE(0, ViE.network->SetSendToS(tbChannel.videoChannel, -1, true));

        
        EXPECT_NE(0, ViE.network->SetSendToS(tbChannel.videoChannel, 64, true));

        
        EXPECT_EQ(0, ViE.network->SetSendToS(tbChannel.videoChannel, 20, true));
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));

        EXPECT_EQ(20, DSCP);
        EXPECT_TRUE(useSetSockOpt);

        
        EXPECT_EQ(0, ViE.network->SetSendToS(tbChannel.videoChannel, 0, true));
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));
        EXPECT_EQ(0, DSCP);

        char myIpAddress[64];
        memset(myIpAddress, 0, 64);
        
        EXPECT_EQ(0, ViE.network->GetLocalIP(myIpAddress, false));
        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, 1234, 1235, myIpAddress));

        
        EXPECT_NE(0, ViE.network->SetSendToS(
            tbChannel.videoChannel, -1, false));
        EXPECT_NE(0, ViE.network->SetSendToS(
            tbChannel.videoChannel, 64, false));  
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));  
        EXPECT_EQ(0, DSCP);
        int tos_result = ViE.network->SetSendToS(
            tbChannel.videoChannel, 20, false);  
        EXPECT_EQ(0, tos_result);
        if (tos_result != 0)
        {
            ViETest::Log("ViESetSendToS error!.");
            ViETest::Log("You must be admin to run these tests.");
            ViETest::Log("On Win7 and late Vista, you need to right click the "
                         "exe and choose");
            ViETest::Log("\"Run as administrator\"\n");
            getchar();
        }
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));
        EXPECT_EQ(20, DSCP);
#ifdef _WIN32
        EXPECT_FALSE(useSetSockOpt);
#else 
        EXPECT_TRUE(useSetSockOpt);
#endif
        EXPECT_EQ(0, ViE.network->SetSendToS(tbChannel.videoChannel, 0, false));
        EXPECT_EQ(0, ViE.network->GetSendToS(
            tbChannel.videoChannel, DSCP, useSetSockOpt));
        EXPECT_EQ(0, DSCP);
    }
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        TbVideoChannel tbChannel(ViE);  


#if defined(_WIN32)
        
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_BESTEFFORT));

        EXPECT_EQ(0, ViE.network->SetLocalReceiver(
            tbChannel.videoChannel, 1234));

        
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_BESTEFFORT));
        EXPECT_EQ(0, ViE.network->SetSendDestination(
            tbChannel.videoChannel, "127.0.0.1", 12345));

        
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_NOTRAFFIC));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_NETWORK_UNAVAILABLE));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_GENERAL_INFORMATION));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_NOCHANGE));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_NONCONFORMING));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_NOTRAFFIC));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_NETWORK_CONTROL));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICE_BESTEFFORT));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICE_CONTROLLEDLOAD));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICE_GUARANTEED));
        EXPECT_NE(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICE_QUALITATIVE));

        
        bool enabled = false;
        int serviceType = 0;
        int overrideDSCP = 0;

        EXPECT_EQ(0, ViE.network->GetSendGQoS(
            tbChannel.videoChannel, enabled, serviceType, overrideDSCP));
        EXPECT_FALSE(enabled);
        EXPECT_EQ(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_BESTEFFORT));
        EXPECT_EQ(0, ViE.network->GetSendGQoS(
            tbChannel.videoChannel, enabled, serviceType, overrideDSCP));
        EXPECT_TRUE(enabled);
        EXPECT_EQ(SERVICETYPE_BESTEFFORT, serviceType);
        EXPECT_FALSE(overrideDSCP);

        EXPECT_EQ(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_CONTROLLEDLOAD));
        EXPECT_EQ(0, ViE.network->GetSendGQoS(
            tbChannel.videoChannel, enabled, serviceType, overrideDSCP));
        EXPECT_TRUE(enabled);
        EXPECT_EQ(SERVICETYPE_CONTROLLEDLOAD, serviceType);
        EXPECT_FALSE(overrideDSCP);

        EXPECT_EQ(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_GUARANTEED));
        EXPECT_EQ(0, ViE.network->GetSendGQoS(
            tbChannel.videoChannel, enabled, serviceType, overrideDSCP));
        EXPECT_TRUE(enabled);
        EXPECT_EQ(SERVICETYPE_GUARANTEED, serviceType);
        EXPECT_FALSE(overrideDSCP);

        EXPECT_EQ(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, true, SERVICETYPE_QUALITATIVE));
        EXPECT_EQ(0, ViE.network->GetSendGQoS(
            tbChannel.videoChannel, enabled, serviceType, overrideDSCP));
        EXPECT_TRUE(enabled);
        EXPECT_EQ(SERVICETYPE_QUALITATIVE, serviceType);
        EXPECT_FALSE(overrideDSCP);

        EXPECT_EQ(0, ViE.network->SetSendGQoS(
            tbChannel.videoChannel, false, SERVICETYPE_QUALITATIVE));
        EXPECT_EQ(0, ViE.network->GetSendGQoS(
            tbChannel.videoChannel, enabled, serviceType, overrideDSCP));
        EXPECT_FALSE(enabled);
#endif
    }
    {
        
        
        
        
        TbVideoChannel tbChannel(ViE);
        
        EXPECT_NE(0, ViE.network->SetMTU(tbChannel.videoChannel, 1600));
        
        EXPECT_EQ(0, ViE.network->SetMTU(tbChannel.videoChannel, 800));

        
        
        
        ViEAutoTestNetworkObserver vieTestObserver;
        EXPECT_EQ(0, ViE.network->RegisterObserver(
            tbChannel.videoChannel, vieTestObserver));
        EXPECT_NE(0, ViE.network->RegisterObserver(
            tbChannel.videoChannel, vieTestObserver));
        EXPECT_EQ(0, ViE.network->SetPeriodicDeadOrAliveStatus(
            tbChannel.videoChannel, true)); 
        EXPECT_EQ(0, ViE.network->DeregisterObserver(tbChannel.videoChannel));

        EXPECT_NE(0, ViE.network->DeregisterObserver(tbChannel.videoChannel));
        EXPECT_NE(0, ViE.network->SetPeriodicDeadOrAliveStatus(
            tbChannel.videoChannel, true)); 

        
        EXPECT_EQ(0, ViE.network->SetPacketTimeoutNotification(
            tbChannel.videoChannel, true, 10));
    }

    
    
    
}
