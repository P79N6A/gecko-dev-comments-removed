












#include <iostream>

#include "engine_configurations.h"
#include "tb_capture_device.h"
#include "tb_external_transport.h"
#include "tb_interfaces.h"
#include "tb_video_channel.h"
#include "testsupport/fileutils.h"
#include "vie_autotest.h"
#include "vie_autotest_defines.h"

class ViERtpObserver: public webrtc::ViERTPObserver
{
public:
    ViERtpObserver()
    {
    }
    virtual ~ViERtpObserver()
    {
    }

    virtual void IncomingSSRCChanged(const int videoChannel,
                                     const unsigned int SSRC)
    {
    }
    virtual void IncomingCSRCChanged(const int videoChannel,
                                     const unsigned int CSRC, const bool added)
    {
    }
};

class ViERtcpObserver: public webrtc::ViERTCPObserver
{
public:
    int _channel;
    unsigned char _subType;
    unsigned int _name;
    char* _data;
    unsigned short _dataLength;

    ViERtcpObserver() :
        _channel(-1),
        _subType(0),
        _name(-1),
        _data(NULL),
        _dataLength(0)
    {
    }
    ~ViERtcpObserver()
    {
        if (_data)
        {
            delete[] _data;
        }
    }
    virtual void OnApplicationDataReceived(
        const int videoChannel, const unsigned char subType,
        const unsigned int name, const char* data,
        const unsigned short dataLengthInBytes)
    {
        _channel = videoChannel;
        _subType = subType;
        _name = name;
        if (dataLengthInBytes > _dataLength)
        {
            delete[] _data;
            _data = NULL;
        }
        if (_data == NULL)
        {
            _data = new char[dataLengthInBytes];
        }
        memcpy(_data, data, dataLengthInBytes);
        _dataLength = dataLengthInBytes;
    }
};

void ViEAutoTest::ViERtpRtcpStandardTest()
{
    
    
    

    
    TbInterfaces ViE("ViERtpRtcpStandardTest");
    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);

    
    TbCaptureDevice tbCapture(ViE);
    tbCapture.ConnectTo(tbChannel.videoChannel);

    ViETest::Log("\n");
    TbExternalTransport myTransport(*(ViE.network));

    EXPECT_EQ(0, ViE.network->RegisterSendTransport(
        tbChannel.videoChannel, myTransport));

    
    
    
    unsigned short startSequenceNumber = 12345;
    ViETest::Log("Set start sequence number: %u", startSequenceNumber);
    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, startSequenceNumber));

    myTransport.EnableSequenceNumberCheck();

    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    AutoTestSleep(2000);

    unsigned short receivedSequenceNumber =
        myTransport.GetFirstSequenceNumber();
    ViETest::Log("First received sequence number: %u\n",
                 receivedSequenceNumber);
    EXPECT_EQ(startSequenceNumber, receivedSequenceNumber);

    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    
    
    
    ViETest::Log("Testing CName\n");
    const char* sendCName = "ViEAutoTestCName\0";
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRTCPCName(tbChannel.videoChannel, sendCName));

    char returnCName[webrtc::ViERTP_RTCP::KMaxRTCPCNameLength];
    memset(returnCName, 0, webrtc::ViERTP_RTCP::KMaxRTCPCNameLength);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRTCPCName(
        tbChannel.videoChannel, returnCName));
    EXPECT_STRCASEEQ(sendCName, returnCName);

    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    AutoTestSleep(1000);

    char remoteCName[webrtc::ViERTP_RTCP::KMaxRTCPCNameLength];
    memset(remoteCName, 0, webrtc::ViERTP_RTCP::KMaxRTCPCNameLength);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRemoteRTCPCName(
        tbChannel.videoChannel, remoteCName));
    EXPECT_STRCASEEQ(sendCName, remoteCName);

    
    
    
    
    ViETest::Log("Testing statistics\n");
    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    myTransport.ClearStats();
    int rate = 20;
    myTransport.SetPacketLoss(rate);

    

    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, startSequenceNumber));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));

    AutoTestSleep(KAutoTestSleepTimeMs);

    unsigned short sentFractionsLost = 0;
    unsigned int sentCumulativeLost = 0;
    unsigned int sentExtendedMax = 0;
    unsigned int sentJitter = 0;
    int sentRttMs = 0;
    unsigned short recFractionsLost = 0;
    unsigned int recCumulativeLost = 0;
    unsigned int recExtendedMax = 0;
    unsigned int recJitter = 0;
    int recRttMs = 0;

    unsigned int sentTotalBitrate = 0;
    unsigned int sentVideoBitrate = 0;
    unsigned int sentFecBitrate = 0;
    unsigned int sentNackBitrate = 0;

    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
        sentFecBitrate, sentNackBitrate));

    EXPECT_GT(sentTotalBitrate, 0u);
    EXPECT_EQ(sentFecBitrate, 0u);
    EXPECT_EQ(sentNackBitrate, 0u);

    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));

    AutoTestSleep(2000);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetSentRTCPStatistics(
        tbChannel.videoChannel, sentFractionsLost, sentCumulativeLost,
        sentExtendedMax, sentJitter, sentRttMs));
    EXPECT_GT(sentCumulativeLost, 0u);
    EXPECT_GT(sentExtendedMax, startSequenceNumber);
    EXPECT_GT(sentJitter, 0u);
    EXPECT_GT(sentRttMs, 0);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetReceivedRTCPStatistics(
        tbChannel.videoChannel, recFractionsLost, recCumulativeLost,
        recExtendedMax, recJitter, recRttMs));

    EXPECT_GT(recCumulativeLost, 0u);
    EXPECT_GT(recExtendedMax, startSequenceNumber);
    EXPECT_GT(recJitter, 0u);
    EXPECT_GT(recRttMs, 0);

    unsigned int estimated_bandwidth = 0;
    EXPECT_EQ(0, ViE.rtp_rtcp->GetEstimatedSendBandwidth(
        tbChannel.videoChannel,
        &estimated_bandwidth));
    EXPECT_GT(estimated_bandwidth, 0u);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetEstimatedReceiveBandwidth(
        tbChannel.videoChannel,
        &estimated_bandwidth));
    EXPECT_GT(estimated_bandwidth, 0u);

    
    EXPECT_GE(recExtendedMax, sentExtendedMax);
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    
    
    

    myTransport.ClearStats();
    myTransport.SetPacketLoss(rate);

    EXPECT_EQ(0, ViE.rtp_rtcp->SetFECStatus(
        tbChannel.videoChannel, true, 96, 97));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
         sentFecBitrate, sentNackBitrate));

    EXPECT_GT(sentTotalBitrate, 0u);
    EXPECT_GE(sentFecBitrate, 10u);
    EXPECT_EQ(sentNackBitrate, 0u);

    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetFECStatus(
        tbChannel.videoChannel, false, 96, 97));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
        sentFecBitrate, sentNackBitrate));

    
    
    
    
    
    

    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, false));


    
    myTransport.SetPacketLoss(0);
    myTransport.ClearStats();

    unsigned int setSSRC = 0x01234567;
    ViETest::Log("Set SSRC %u", setSSRC);
    EXPECT_EQ(0, ViE.rtp_rtcp->SetLocalSSRC(tbChannel.videoChannel, setSSRC));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    myTransport.EnableSSRCCheck();

    AutoTestSleep(2000);
    unsigned int receivedSSRC = myTransport.ReceivedSSRC();
    ViETest::Log("Received SSRC %u\n", receivedSSRC);
    EXPECT_EQ(setSSRC, receivedSSRC);

    unsigned int localSSRC = 0;
    EXPECT_EQ(0, ViE.rtp_rtcp->GetLocalSSRC(tbChannel.videoChannel, localSSRC));
    EXPECT_EQ(setSSRC, localSSRC);

    unsigned int remoteSSRC = 0;
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRemoteSSRC(
        tbChannel.videoChannel, remoteSSRC));
    EXPECT_EQ(setSSRC, remoteSSRC);

    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    ViETest::Log("Testing RTP dump...\n");

    std::string inDumpName =
        ViETest::GetResultOutputPath() + "IncomingRTPDump.rtp";
    std::string outDumpName =
        ViETest::GetResultOutputPath() + "OutgoingRTPDump.rtp";
    EXPECT_EQ(0, ViE.rtp_rtcp->StartRTPDump(
        tbChannel.videoChannel, inDumpName.c_str(), webrtc::kRtpIncoming));
    EXPECT_EQ(0, ViE.rtp_rtcp->StartRTPDump(
        tbChannel.videoChannel, outDumpName.c_str(), webrtc::kRtpOutgoing));

    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    AutoTestSleep(1000);

    EXPECT_EQ(0, ViE.rtp_rtcp->StopRTPDump(
        tbChannel.videoChannel, webrtc::kRtpIncoming));
    EXPECT_EQ(0, ViE.rtp_rtcp->StopRTPDump(
        tbChannel.videoChannel, webrtc::kRtpOutgoing));

    
    
    FILE* inDump = fopen(inDumpName.c_str(), "r");
    fseek(inDump, 0L, SEEK_END);
    long inEndPos = ftell(inDump);
    fclose(inDump);
    FILE* outDump = fopen(outDumpName.c_str(), "r");
    fseek(outDump, 0L, SEEK_END);
    long outEndPos = ftell(outDump);
    fclose(outDump);

    EXPECT_GT(inEndPos, 0);
    EXPECT_LT(inEndPos, outEndPos + 100);

    
    EXPECT_EQ(0, ViE.network->DeregisterSendTransport(tbChannel.videoChannel));

    
    
    
    
    if (tbCapture.device_name() != "vivi") {
      
      
      
      
      
      
      const unsigned int start_rate_1_bps = 100000;
      const unsigned int start_rate_2_bps = 300000;
      const unsigned int start_rate_3_bps = 1000000;

      int channel_1 = -1;
      int channel_2 = -1;
      int channel_3 = -1;
      EXPECT_EQ(0, ViE.base->CreateChannel(channel_1));
      EXPECT_EQ(0, ViE.base->CreateChannel(channel_2, channel_1));
      EXPECT_EQ(0, ViE.base->CreateChannel(channel_3));

      
      tbCapture.ConnectTo(channel_1);
      tbCapture.ConnectTo(channel_2);
      tbCapture.ConnectTo(channel_3);

      TbExternalTransport transport_1(*(ViE.network));
      TbExternalTransport transport_2(*(ViE.network));
      TbExternalTransport transport_3(*(ViE.network));

      EXPECT_EQ(0, ViE.network->RegisterSendTransport(channel_1, transport_1));
      EXPECT_EQ(0, ViE.network->RegisterSendTransport(channel_2, transport_2));
      EXPECT_EQ(0, ViE.network->RegisterSendTransport(channel_3, transport_3));

      webrtc::VideoCodec video_codec;
      for (int idx = 0; idx < ViE.codec->NumberOfCodecs(); ++idx) {
        ViE.codec->GetCodec(idx, video_codec);
        if (video_codec.codecType == webrtc::kVideoCodecVP8) {
          break;
        }
      }
      EXPECT_EQ(0, ViE.codec->SetReceiveCodec(channel_1, video_codec));
      EXPECT_EQ(0, ViE.codec->SetReceiveCodec(channel_2, video_codec));
      EXPECT_EQ(0, ViE.codec->SetReceiveCodec(channel_3, video_codec));

      video_codec.startBitrate = start_rate_1_bps / 1000;
      EXPECT_EQ(0, ViE.codec->SetSendCodec(channel_1, video_codec));
      video_codec.startBitrate = start_rate_2_bps / 1000;
      EXPECT_EQ(0, ViE.codec->SetSendCodec(channel_2, video_codec));
      video_codec.startBitrate = start_rate_3_bps / 1000;
      EXPECT_EQ(0, ViE.codec->SetSendCodec(channel_3, video_codec));

      EXPECT_EQ(0, ViE.rtp_rtcp->SetRembStatus(channel_1, true, true));
      EXPECT_EQ(0, ViE.rtp_rtcp->SetRembStatus(channel_2, true, true));
      EXPECT_EQ(0, ViE.rtp_rtcp->SetRembStatus(channel_3, true, true));

      EXPECT_EQ(0, ViE.base->StartReceive(channel_1));
      EXPECT_EQ(0, ViE.base->StartReceive(channel_2));
      EXPECT_EQ(0, ViE.base->StartReceive(channel_3));
      EXPECT_EQ(0, ViE.base->StartSend(channel_1));
      EXPECT_EQ(0, ViE.base->StartSend(channel_2));
      EXPECT_EQ(0, ViE.base->StartSend(channel_3));

      AutoTestSleep(KAutoTestSleepTimeMs);

      EXPECT_EQ(0, ViE.base->StopReceive(channel_1));
      EXPECT_EQ(0, ViE.base->StopReceive(channel_2));
      EXPECT_EQ(0, ViE.base->StopReceive(channel_3));
      EXPECT_EQ(0, ViE.base->StopSend(channel_1));
      EXPECT_EQ(0, ViE.base->StopSend(channel_2));
      EXPECT_EQ(0, ViE.base->StopSend(channel_3));

      unsigned int bw_estimate_1 = 0;
      unsigned int bw_estimate_2 = 0;
      unsigned int bw_estimate_3 = 0;
      ViE.rtp_rtcp->GetEstimatedSendBandwidth(channel_1, &bw_estimate_1);
      ViE.rtp_rtcp->GetEstimatedSendBandwidth(channel_2, &bw_estimate_2);
      ViE.rtp_rtcp->GetEstimatedSendBandwidth(channel_3, &bw_estimate_3);

      EXPECT_LT(bw_estimate_1, start_rate_2_bps);
      EXPECT_LT(bw_estimate_2, start_rate_2_bps);
      EXPECT_NE(bw_estimate_1, start_rate_1_bps);

      
      EXPECT_GT(bw_estimate_3, 0.75 * start_rate_3_bps);

      EXPECT_EQ(0, ViE.base->DeleteChannel(channel_1));
      EXPECT_EQ(0, ViE.base->DeleteChannel(channel_2));
      EXPECT_EQ(0, ViE.base->DeleteChannel(channel_3));
    }

    
    
    
}

void ViEAutoTest::ViERtpRtcpExtendedTest()
{
    
    
    
    ViERtpRtcpStandardTest();

    
    TbInterfaces ViE("ViERtpRtcpExtendedTest");
    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
    
    TbCaptureDevice tbCapture(ViE);
    tbCapture.ConnectTo(tbChannel.videoChannel);

    
    
    TbExternalTransport myTransport(*(ViE.network));

    EXPECT_EQ(0, ViE.network->RegisterSendTransport(
        tbChannel.videoChannel, myTransport));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    
    
    

    
    
    
    

    ViERtcpObserver rtcpObserver;
    EXPECT_EQ(0, ViE.rtp_rtcp->RegisterRTCPObserver(
        tbChannel.videoChannel, rtcpObserver));

    unsigned char subType = 3;
    unsigned int name = static_cast<unsigned int> (0x41424344); 
    const char* data = "ViEAutoTest Data of length 32 -\0";
    const unsigned short numBytes = 32;

    EXPECT_EQ(0, ViE.rtp_rtcp->SendApplicationDefinedRTCPPacket(
        tbChannel.videoChannel, subType, name, data, numBytes));

    ViETest::Log("Sending RTCP application data...\n");
    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(subType, rtcpObserver._subType);
    EXPECT_STRCASEEQ(data, rtcpObserver._data);
    EXPECT_EQ(name, rtcpObserver._name);
    EXPECT_EQ(numBytes, rtcpObserver._dataLength);

    ViETest::Log("\t RTCP application data received\n");

    
    
    
    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    EXPECT_EQ(0, ViE.network->DeregisterSendTransport(tbChannel.videoChannel));
}

void ViEAutoTest::ViERtpRtcpAPITest()
{
    
    
    
    
    TbInterfaces ViE("ViERtpRtcpAPITest");
    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
    
    TbCaptureDevice tbCapture(ViE);
    tbCapture.ConnectTo(tbChannel.videoChannel);

    
    
    

    
    
    
    webrtc::ViERTCPMode rtcpMode = webrtc::kRtcpNone;
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRTCPStatus(
        tbChannel.videoChannel, rtcpMode));
    EXPECT_EQ(webrtc::kRtcpCompound_RFC4585, rtcpMode);
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRTCPStatus(
        tbChannel.videoChannel, webrtc::kRtcpCompound_RFC4585));
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRTCPStatus(
        tbChannel.videoChannel, rtcpMode));
    EXPECT_EQ(webrtc::kRtcpCompound_RFC4585, rtcpMode);
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRTCPStatus(
        tbChannel.videoChannel, webrtc::kRtcpNonCompound_RFC5506));
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRTCPStatus(
        tbChannel.videoChannel, rtcpMode));
    EXPECT_EQ(webrtc::kRtcpNonCompound_RFC5506, rtcpMode);
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRTCPStatus(
        tbChannel.videoChannel, webrtc::kRtcpNone));
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRTCPStatus(
        tbChannel.videoChannel, rtcpMode));
    EXPECT_EQ(webrtc::kRtcpNone, rtcpMode);
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRTCPStatus(
        tbChannel.videoChannel, webrtc::kRtcpCompound_RFC4585));

    
    
    
    
    const char* testCName = "ViEAutotestCName";
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRTCPCName(
        tbChannel.videoChannel, testCName));

    char returnCName[256];
    memset(returnCName, 0, 256);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRTCPCName(
        tbChannel.videoChannel, returnCName));
    EXPECT_STRCASEEQ(testCName, returnCName);

    
    
    
    EXPECT_EQ(0, ViE.rtp_rtcp->SetLocalSSRC(
        tbChannel.videoChannel, 0x01234567));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetLocalSSRC(
        tbChannel.videoChannel, 0x76543210));

    unsigned int ssrc = 0;
    EXPECT_EQ(0, ViE.rtp_rtcp->GetLocalSSRC(tbChannel.videoChannel, ssrc));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, 1000));
    tbChannel.StartSend();
    EXPECT_NE(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, 12345));
    tbChannel.StopSend();

    
    
    
    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, 12345));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, 1000));
    tbChannel.StartSend();
    EXPECT_NE(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, 12345));
    tbChannel.StopSend();

    
    
    
    {
        unsigned char subType = 3;
        unsigned int name = static_cast<unsigned int> (0x41424344); 
        const char* data = "ViEAutoTest Data of length 32 --";
        const unsigned short numBytes = 32;

        tbChannel.StartSend();
        EXPECT_EQ(0, ViE.rtp_rtcp->SendApplicationDefinedRTCPPacket(
            tbChannel.videoChannel, subType, name, data, numBytes));
        EXPECT_NE(0, ViE.rtp_rtcp->SendApplicationDefinedRTCPPacket(
            tbChannel.videoChannel, subType, name, NULL, numBytes)) <<
                "Should fail on NULL input.";
        EXPECT_NE(0, ViE.rtp_rtcp->SendApplicationDefinedRTCPPacket(
            tbChannel.videoChannel, subType, name, data, numBytes - 1)) <<
                "Should fail on incorrect length.";

        EXPECT_EQ(0, ViE.rtp_rtcp->GetRTCPStatus(
            tbChannel.videoChannel, rtcpMode));
        EXPECT_EQ(0, ViE.rtp_rtcp->SendApplicationDefinedRTCPPacket(
            tbChannel.videoChannel, subType, name, data, numBytes));
        EXPECT_EQ(0, ViE.rtp_rtcp->SetRTCPStatus(
            tbChannel.videoChannel, webrtc::kRtcpCompound_RFC4585));
        tbChannel.StopSend();
        EXPECT_NE(0, ViE.rtp_rtcp->SendApplicationDefinedRTCPPacket(
            tbChannel.videoChannel, subType, name, data, numBytes));
    }

    
    
    
    
    

    
    
    
    {
        std::string output_file = webrtc::test::OutputPath() +
            "DumpFileName.rtp";
        const char* dumpName = output_file.c_str();

        EXPECT_EQ(0, ViE.rtp_rtcp->StartRTPDump(
            tbChannel.videoChannel, dumpName, webrtc::kRtpIncoming));
        EXPECT_EQ(0, ViE.rtp_rtcp->StopRTPDump(
            tbChannel.videoChannel, webrtc::kRtpIncoming));
        EXPECT_NE(0, ViE.rtp_rtcp->StopRTPDump(
            tbChannel.videoChannel, webrtc::kRtpIncoming));
        EXPECT_EQ(0, ViE.rtp_rtcp->StartRTPDump(
            tbChannel.videoChannel, dumpName, webrtc::kRtpOutgoing));
        EXPECT_EQ(0, ViE.rtp_rtcp->StopRTPDump(
            tbChannel.videoChannel, webrtc::kRtpOutgoing));
        EXPECT_NE(0, ViE.rtp_rtcp->StopRTPDump(
            tbChannel.videoChannel, webrtc::kRtpOutgoing));
        EXPECT_NE(0, ViE.rtp_rtcp->StartRTPDump(
            tbChannel.videoChannel, dumpName, (webrtc::RTPDirections) 3));
    }
    
    
    
    {
        ViERtpObserver rtpObserver;
        EXPECT_EQ(0, ViE.rtp_rtcp->RegisterRTPObserver(
            tbChannel.videoChannel, rtpObserver));
        EXPECT_NE(0, ViE.rtp_rtcp->RegisterRTPObserver(
            tbChannel.videoChannel, rtpObserver));
        EXPECT_EQ(0, ViE.rtp_rtcp->DeregisterRTPObserver(
            tbChannel.videoChannel));
        EXPECT_NE(0, ViE.rtp_rtcp->DeregisterRTPObserver(
            tbChannel.videoChannel));

        ViERtcpObserver rtcpObserver;
        EXPECT_EQ(0, ViE.rtp_rtcp->RegisterRTCPObserver(
            tbChannel.videoChannel, rtcpObserver));
        EXPECT_NE(0, ViE.rtp_rtcp->RegisterRTCPObserver(
            tbChannel.videoChannel, rtcpObserver));
        EXPECT_EQ(0, ViE.rtp_rtcp->DeregisterRTCPObserver(
            tbChannel.videoChannel));
        EXPECT_NE(0, ViE.rtp_rtcp->DeregisterRTCPObserver(
            tbChannel.videoChannel));
    }
    
    
    
    {
        EXPECT_EQ(0, ViE.rtp_rtcp->SetKeyFrameRequestMethod(
            tbChannel.videoChannel, webrtc::kViEKeyFrameRequestPliRtcp));
        EXPECT_EQ(0, ViE.rtp_rtcp->SetKeyFrameRequestMethod(
            tbChannel.videoChannel, webrtc::kViEKeyFrameRequestPliRtcp));
        EXPECT_EQ(0, ViE.rtp_rtcp->SetKeyFrameRequestMethod(
            tbChannel.videoChannel, webrtc::kViEKeyFrameRequestNone));
        EXPECT_EQ(0, ViE.rtp_rtcp->SetKeyFrameRequestMethod(
            tbChannel.videoChannel, webrtc::kViEKeyFrameRequestNone));
    }
    
    
    
    {
      EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, true));
    }

    
    
    
}
