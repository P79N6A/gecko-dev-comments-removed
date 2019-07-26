









#include <iostream>

#include "webrtc/engine_configurations.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_defines.h"
#include "webrtc/video_engine/test/libvietest/include/tb_capture_device.h"
#include "webrtc/video_engine/test/libvietest/include/tb_external_transport.h"
#include "webrtc/video_engine/test/libvietest/include/tb_interfaces.h"
#include "webrtc/video_engine/test/libvietest/include/tb_video_channel.h"

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
    TbExternalTransport myTransport(*(ViE.network), tbChannel.videoChannel,
                                    NULL);

    ViE.network->DeregisterSendTransport(tbChannel.videoChannel);
    EXPECT_EQ(0, ViE.network->RegisterSendTransport(
        tbChannel.videoChannel, myTransport));

    
    
    
    unsigned short startSequenceNumber = 12345;
    ViETest::Log("Set start sequence number: %u", startSequenceNumber);
    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, startSequenceNumber));
    const unsigned int kVideoSsrc = 123456;
    
    EXPECT_EQ(0, ViE.rtp_rtcp->SetLocalSSRC(tbChannel.videoChannel, kVideoSsrc,
                                            webrtc::kViEStreamTypeNormal, 0));

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

    if (FLAGS_include_timing_dependent_tests) {
      char remoteCName[webrtc::ViERTP_RTCP::KMaxRTCPCNameLength];
      memset(remoteCName, 0, webrtc::ViERTP_RTCP::KMaxRTCPCNameLength);
      EXPECT_EQ(0, ViE.rtp_rtcp->GetRemoteRTCPCName(
          tbChannel.videoChannel, remoteCName));
      EXPECT_STRCASEEQ(sendCName, remoteCName);
    }


    
    
    
    webrtc::RtcpStatistics received;
    int recRttMs = 0;
    unsigned int sentTotalBitrate = 0;
    unsigned int sentVideoBitrate = 0;
    unsigned int sentFecBitrate = 0;
    unsigned int sentNackBitrate = 0;

    ViETest::Log("Testing Pacing\n");
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));

    myTransport.ClearStats();

    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    NetworkParameters network;
    network.packet_loss_rate = 0;
    network.loss_model = kUniformLoss;
    myTransport.SetNetworkParameters(network);

    AutoTestSleep(kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetReceiveChannelRtcpStatistics(
        tbChannel.videoChannel, received, recRttMs));
    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
        sentFecBitrate, sentNackBitrate));

    int num_rtp_packets = 0;
    int num_dropped_packets = 0;
    int num_rtcp_packets = 0;
    std::map<uint8_t, int> packet_counters;
    myTransport.GetStats(num_rtp_packets, num_dropped_packets, num_rtcp_packets,
                         &packet_counters);
    EXPECT_GT(num_rtp_packets, 0);
    EXPECT_EQ(num_dropped_packets, 0);
    EXPECT_GT(num_rtcp_packets, 0);
    EXPECT_GT(sentTotalBitrate, 0u);
    EXPECT_EQ(sentNackBitrate, 0u);
    EXPECT_EQ(received.cumulative_lost, 0u);

    
    
    
    ViETest::Log("Testing NACK over RTX\n");
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));

    myTransport.ClearStats();

    const uint8_t kRtxPayloadType = 96;
    EXPECT_EQ(0, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        tbChannel.videoChannel, false));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRtxSendPayloadType(tbChannel.videoChannel,
                                                     kRtxPayloadType));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRtxReceivePayloadType(tbChannel.videoChannel,
                                                        kRtxPayloadType));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetLocalSSRC(tbChannel.videoChannel, 1234,
                                            webrtc::kViEStreamTypeRtx, 0));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetRemoteSSRCType(tbChannel.videoChannel,
                                                 webrtc::kViEStreamTypeRtx,
                                                 1234));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, startSequenceNumber));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    
    AutoTestSleep(100);
    const int kPacketLossRate = 20;
    network.packet_loss_rate = kPacketLossRate;
    network.loss_model = kUniformLoss;
    myTransport.SetNetworkParameters(network);
    AutoTestSleep(kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetReceiveChannelRtcpStatistics(
        tbChannel.videoChannel, received, recRttMs));
    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
        sentFecBitrate, sentNackBitrate));

    packet_counters.clear();
    myTransport.GetStats(num_rtp_packets, num_dropped_packets, num_rtcp_packets,
                         &packet_counters);
    EXPECT_GT(num_rtp_packets, 0);
    EXPECT_GT(num_dropped_packets, 0);
    EXPECT_GT(num_rtcp_packets, 0);
    EXPECT_GT(packet_counters[kRtxPayloadType], 0);

    
    
    
    EXPECT_GT(sentTotalBitrate, 0u);
    EXPECT_GT(sentNackBitrate, 0u);

    
    
    
    
    ViETest::Log("Testing statistics\n");
    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, false));
    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    myTransport.ClearStats();
    network.packet_loss_rate = kPacketLossRate;
    network.loss_model = kUniformLoss;
    myTransport.SetNetworkParameters(network);

    

    EXPECT_EQ(0, ViE.rtp_rtcp->SetStartSequenceNumber(
        tbChannel.videoChannel, startSequenceNumber));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    webrtc::RtcpStatistics sent;
    int sentRttMs = 0;

    
    
    
    int time_to_sleep = kAutoTestSleepTimeMs;
    bool got_send_channel_frac_lost = false;
    bool got_receive_channel_frac_lost = false;
    while (time_to_sleep > 0) {
      AutoTestSleep(500);
      time_to_sleep -= 500;
      EXPECT_EQ(0,
                ViE.rtp_rtcp->GetSendChannelRtcpStatistics(
                    tbChannel.videoChannel, sent, sentRttMs));
      got_send_channel_frac_lost |= sent.fraction_lost > 0;
      EXPECT_EQ(0,
                ViE.rtp_rtcp->GetReceiveChannelRtcpStatistics(
                    tbChannel.videoChannel, received, recRttMs));
      got_receive_channel_frac_lost |= received.fraction_lost > 0;
    }
    EXPECT_TRUE(got_send_channel_frac_lost);
    EXPECT_TRUE(got_receive_channel_frac_lost);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
        sentFecBitrate, sentNackBitrate));

    EXPECT_GT(sentTotalBitrate, 0u);
    EXPECT_EQ(sentFecBitrate, 0u);
    EXPECT_EQ(sentNackBitrate, 0u);

    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));

    AutoTestSleep(2000);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetSendChannelRtcpStatistics(
        tbChannel.videoChannel, sent, sentRttMs));
    EXPECT_GT(sent.cumulative_lost, 0u);
    EXPECT_GT(sent.extended_max_sequence_number, startSequenceNumber);
    EXPECT_GT(sent.jitter, 0u);
    EXPECT_GT(sentRttMs, 0);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetReceiveChannelRtcpStatistics(
        tbChannel.videoChannel, received, recRttMs));

    EXPECT_GT(received.cumulative_lost, 0u);
    EXPECT_GT(received.extended_max_sequence_number, startSequenceNumber);
    EXPECT_GT(received.jitter, 0u);
    EXPECT_GT(recRttMs, 0);

    unsigned int estimated_bandwidth = 0;
    EXPECT_EQ(0, ViE.rtp_rtcp->GetEstimatedSendBandwidth(
        tbChannel.videoChannel,
        &estimated_bandwidth));
    EXPECT_GT(estimated_bandwidth, 0u);

    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_EQ(0, ViE.rtp_rtcp->GetEstimatedReceiveBandwidth(
          tbChannel.videoChannel,
          &estimated_bandwidth));
      EXPECT_GT(estimated_bandwidth, 0u);

      int passive_channel = -1;
      EXPECT_EQ(ViE.base->CreateReceiveChannel(passive_channel,
                                               tbChannel.videoChannel), 0);
      EXPECT_EQ(ViE.base->StartReceive(passive_channel), 0);
      EXPECT_EQ(
          ViE.rtp_rtcp->GetEstimatedReceiveBandwidth(passive_channel,
                                                     &estimated_bandwidth),
          0);
      EXPECT_EQ(estimated_bandwidth, 0u);
    }

    
    EXPECT_GE(received.extended_max_sequence_number,
              sent.extended_max_sequence_number);
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));

    
    
    

    myTransport.ClearStats();
    network.packet_loss_rate = kPacketLossRate;
    myTransport.SetNetworkParameters(network);

    EXPECT_EQ(0, ViE.rtp_rtcp->SetFECStatus(
        tbChannel.videoChannel, true, 96, 97));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    AutoTestSleep(kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
         sentFecBitrate, sentNackBitrate));

    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_GT(sentTotalBitrate, 0u);
      EXPECT_GT(sentFecBitrate, 0u);
      EXPECT_EQ(sentNackBitrate, 0u);
    }

    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetFECStatus(
        tbChannel.videoChannel, false, 96, 97));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    AutoTestSleep(4 * kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.rtp_rtcp->GetBandwidthUsage(
        tbChannel.videoChannel, sentTotalBitrate, sentVideoBitrate,
        sentFecBitrate, sentNackBitrate));

    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_GT(sentTotalBitrate, 0u);
      EXPECT_EQ(sentFecBitrate, 0u);

      
      
      
    }

    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, false));


    
    network.packet_loss_rate = 0;
    myTransport.SetNetworkParameters(network);
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

    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_EQ(setSSRC, receivedSSRC);

      unsigned int localSSRC = 0;
      EXPECT_EQ(0, ViE.rtp_rtcp->GetLocalSSRC(
          tbChannel.videoChannel, localSSRC));
      EXPECT_EQ(setSSRC, localSSRC);

      unsigned int remoteSSRC = 0;
      EXPECT_EQ(0, ViE.rtp_rtcp->GetRemoteSSRC(
          tbChannel.videoChannel, remoteSSRC));
      EXPECT_EQ(setSSRC, remoteSSRC);
    }

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

    AutoTestSleep(kAutoTestSleepTimeMs);

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
    
    fclose(outDump);

    EXPECT_GT(inEndPos, 0);

    
    
    

    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));

    ViETest::Log("Testing Network Down...\n");

    EXPECT_EQ(0, ViE.rtp_rtcp->SetNACKStatus(tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        tbChannel.videoChannel, true));

    webrtc::StreamDataCounters sent_before;
    webrtc::StreamDataCounters received_before;
    webrtc::StreamDataCounters sent_after;
    webrtc::StreamDataCounters received_after;

    EXPECT_EQ(0, ViE.rtp_rtcp->GetRtpStatistics(tbChannel.videoChannel,
                                                sent_before,
                                                received_before));
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));

    
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRtpStatistics(tbChannel.videoChannel,
                                                sent_after, received_after));
    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_GT(received_after.bytes, received_before.bytes);
    }
    
    ViE.network->SetNetworkTransmissionState(tbChannel.videoChannel, false);
    
    
    AutoTestSleep(kAutoTestSleepTimeMs);
    received_before.bytes = received_after.bytes;
    ViETest::Log("Network Down...\n");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRtpStatistics(tbChannel.videoChannel,
                                                sent_before,
                                                received_before));
    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_EQ(received_before.bytes, received_after.bytes);
    }

    
    ViE.network->SetNetworkTransmissionState(tbChannel.videoChannel, true);
    ViETest::Log("Network Up...\n");
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRtpStatistics(tbChannel.videoChannel,
                                                sent_before,
                                                received_before));
    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_GT(received_before.bytes, received_after.bytes);
    }
    received_after.bytes = received_before.bytes;
    
    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));
    ViE.rtp_rtcp->SetSenderBufferingMode(tbChannel.videoChannel,
                                         kAutoTestSleepTimeMs / 2);
    
    
    
    ViE.rtp_rtcp->SetReceiverBufferingMode(tbChannel.videoChannel,
                                           3 * kAutoTestSleepTimeMs / 2);
    EXPECT_EQ(0, ViE.base->StartReceive(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StartSend(tbChannel.videoChannel));
    AutoTestSleep(kAutoTestSleepTimeMs);
    
    ViETest::Log("Network Down...\n");
    ViE.network->SetNetworkTransmissionState(tbChannel.videoChannel, false);
    
    
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRtpStatistics(tbChannel.videoChannel,
                                                sent_before,
                                                received_before));
    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_GT(received_before.bytes, received_after.bytes);
    }
    received_after.bytes = received_before.bytes;
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRtpStatistics(tbChannel.videoChannel,
                                                sent_before,
                                                received_before));
    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_EQ(received_after.bytes, received_before.bytes);
    }
    
    ViETest::Log("Network Up...\n");
    ViE.network->SetNetworkTransmissionState(tbChannel.videoChannel, true);
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.rtp_rtcp->GetRtpStatistics(tbChannel.videoChannel,
                                                sent_before,
                                                received_before));
    if (FLAGS_include_timing_dependent_tests) {
      EXPECT_GT(received_before.bytes, received_after.bytes);
    }
    
    
    

    EXPECT_EQ(0, ViE.base->StopSend(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->StopReceive(tbChannel.videoChannel));


    
    EXPECT_EQ(0, ViE.network->DeregisterSendTransport(tbChannel.videoChannel));


    
    
    
}

void ViEAutoTest::ViERtpRtcpExtendedTest()
{
    
    
    
    
    TbInterfaces ViE("ViERtpRtcpExtendedTest");
    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
    
    TbCaptureDevice tbCapture(ViE);
    tbCapture.ConnectTo(tbChannel.videoChannel);

    
    
    TbExternalTransport myTransport(*(ViE.network), tbChannel.videoChannel,
                                    NULL);

    EXPECT_EQ(0, ViE.network->DeregisterSendTransport(tbChannel.videoChannel));
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
    AutoTestSleep(kAutoTestSleepTimeMs);

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

    
    
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
        tbChannel.videoChannel, true, 0));
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
        tbChannel.videoChannel, true, 15));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
            tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
              tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendTimestampOffsetStatus(
            tbChannel.videoChannel, false, 3));

    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
        tbChannel.videoChannel, true, 0));
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
        tbChannel.videoChannel, true, 15));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
            tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
              tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveTimestampOffsetStatus(
            tbChannel.videoChannel, false, 3));

    
    
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 0));
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 15));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSendAbsoluteSendTimeStatus(
        tbChannel.videoChannel, false, 3));

    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 0));
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 15));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, true, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, false, 3));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(
        tbChannel.videoChannel, false, 3));

    
    const int invalid_channel_id = 17;
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        invalid_channel_id, true));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        tbChannel.videoChannel, true));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        tbChannel.videoChannel, false));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetTransmissionSmoothingStatus(
        tbChannel.videoChannel, false));

    
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetSenderBufferingMode(
        invalid_channel_id, 0));
    int invalid_delay = -1;
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetSenderBufferingMode(
        tbChannel.videoChannel, invalid_delay));
    invalid_delay = 15000;
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetSenderBufferingMode(
        tbChannel.videoChannel, invalid_delay));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSenderBufferingMode(
        tbChannel.videoChannel, 5000));

    
    
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiverBufferingMode(
        tbChannel.videoChannel, 0));
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiverBufferingMode(
        tbChannel.videoChannel, 2000));

    
    webrtc::VoiceEngine* voice_engine = webrtc::VoiceEngine::Create();
    EXPECT_TRUE(NULL != voice_engine);
    webrtc::VoEBase* voe_base = webrtc::VoEBase::GetInterface(voice_engine);
    EXPECT_TRUE(NULL != voe_base);
    EXPECT_EQ(0, voe_base->Init());
    int audio_channel = voe_base->CreateChannel();
    EXPECT_NE(-1, audio_channel);
    EXPECT_EQ(0, ViE.base->SetVoiceEngine(voice_engine));
    EXPECT_EQ(0, ViE.base->ConnectAudioChannel(tbChannel.videoChannel,
                                               audio_channel));

    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiverBufferingMode(
        invalid_channel_id, 0));
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiverBufferingMode(
        tbChannel.videoChannel, invalid_delay));
    invalid_delay = 15000;
    EXPECT_EQ(-1, ViE.rtp_rtcp->SetReceiverBufferingMode(
        tbChannel.videoChannel, invalid_delay));
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiverBufferingMode(
        tbChannel.videoChannel, 5000));

    
    EXPECT_EQ(0, ViE.rtp_rtcp->SetSenderBufferingMode(
        tbChannel.videoChannel, 0));
    
    EXPECT_EQ(0, ViE.rtp_rtcp->SetReceiverBufferingMode(
        tbChannel.videoChannel, 0));

    EXPECT_EQ(0, ViE.base->DisconnectAudioChannel(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.base->SetVoiceEngine(NULL));
    EXPECT_EQ(0, voe_base->DeleteChannel(audio_channel));
    voe_base->Release();
    EXPECT_TRUE(webrtc::VoiceEngine::Delete(voice_engine));

    
    
    
}
