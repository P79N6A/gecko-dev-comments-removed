





#include <iostream>

#include "sigslot.h"

#include "logging.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nss.h"
#include "ssl.h"
#include "sslproto.h"

#include "dtlsidentity.h"
#include "mozilla/RefPtr.h"
#include "FakeMediaStreams.h"
#include "FakeMediaStreamsImpl.h"
#include "MediaConduitErrors.h"
#include "MediaConduitInterface.h"
#include "MediaPipeline.h"
#include "MediaPipelineFilter.h"
#include "runnable_utils.h"
#include "transportflow.h"
#include "transportlayerloopback.h"
#include "transportlayerdtls.h"
#include "mozilla/SyncRunnable.h"


#include "mtransport_test_utils.h"
#include "runnable_utils.h"

#include "webrtc/modules/interface/module_common_types.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

using namespace mozilla;
MOZ_MTLOG_MODULE("mediapipeline")

MtransportTestUtils *test_utils;

namespace {

class TransportInfo {
 public:
  TransportInfo() :
    flow_(nullptr),
    loopback_(nullptr),
    dtls_(nullptr) {}

  static void InitAndConnect(TransportInfo &client, TransportInfo &server) {
    client.Init(true);
    server.Init(false);
    client.PushLayers();
    server.PushLayers();
    client.Connect(&server);
    server.Connect(&client);
  }

  void Init(bool client) {
    nsresult res;

    flow_ = new TransportFlow();
    loopback_ = new TransportLayerLoopback();
    dtls_ = new TransportLayerDtls();

    res = loopback_->Init();
    if (res != NS_OK) {
      FreeLayers();
    }
    ASSERT_EQ((nsresult)NS_OK, res);

    std::vector<uint16_t> ciphers;
    ciphers.push_back(SRTP_AES128_CM_HMAC_SHA1_80);
    dtls_->SetSrtpCiphers(ciphers);
    dtls_->SetIdentity(DtlsIdentity::Generate());
    dtls_->SetRole(client ? TransportLayerDtls::CLIENT :
      TransportLayerDtls::SERVER);
    dtls_->SetVerificationAllowAll();
  }

  void PushLayers() {
    nsresult res;

    nsAutoPtr<std::queue<TransportLayer *> > layers(
      new std::queue<TransportLayer *>);
    layers->push(loopback_);
    layers->push(dtls_);
    res = flow_->PushLayers(layers);
    if (res != NS_OK) {
      FreeLayers();
    }
    ASSERT_EQ((nsresult)NS_OK, res);
  }

  void Connect(TransportInfo* peer) {
    MOZ_ASSERT(loopback_);
    MOZ_ASSERT(peer->loopback_);

    loopback_->Connect(peer->loopback_);
  }

  
  
  void FreeLayers() {
    delete loopback_;
    loopback_ = nullptr;
    delete dtls_;
    dtls_ = nullptr;
  }

  void Shutdown() {
    if (loopback_) {
      loopback_->Disconnect();
    }
    loopback_ = nullptr;
    dtls_ = nullptr;
    flow_ = nullptr;
  }

  mozilla::RefPtr<TransportFlow> flow_;
  TransportLayerLoopback *loopback_;
  TransportLayerDtls *dtls_;
};

class TestAgent {
 public:
  TestAgent() :
      audio_config_(109, "opus", 48000, 960, 2, 64000),
      audio_conduit_(mozilla::AudioSessionConduit::Create(nullptr)),
      audio_(),
      audio_pipeline_() {
  }

  static void ConnectRtp(TestAgent *client, TestAgent *server) {
    TransportInfo::InitAndConnect(client->audio_rtp_transport_,
                                  server->audio_rtp_transport_);
  }

  static void ConnectRtcp(TestAgent *client, TestAgent *server) {
    TransportInfo::InitAndConnect(client->audio_rtcp_transport_,
                                  server->audio_rtcp_transport_);
  }

  static void ConnectBundle(TestAgent *client, TestAgent *server) {
    TransportInfo::InitAndConnect(client->bundle_transport_,
                                  server->bundle_transport_);
  }

  virtual void CreatePipelines_s(bool aIsRtcpMux) = 0;

  void Start() {
    nsresult ret;

    MOZ_MTLOG(ML_DEBUG, "Starting");

    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnableRet(audio_->GetStream(), &Fake_MediaStream::Start, &ret));

    ASSERT_TRUE(NS_SUCCEEDED(ret));
  }

  void StopInt() {
    audio_->GetStream()->Stop();
  }

  void Stop() {
    MOZ_MTLOG(ML_DEBUG, "Stopping");

    if (audio_pipeline_)
      audio_pipeline_->ShutdownMedia_m();

    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnable(this, &TestAgent::StopInt));
  }

  void Shutdown_s() {
    audio_rtp_transport_.Shutdown();
    audio_rtcp_transport_.Shutdown();
    bundle_transport_.Shutdown();
    if (audio_pipeline_)
      audio_pipeline_->ShutdownTransport_s();
  }

  void Shutdown() {
    if (audio_pipeline_)
      audio_pipeline_->ShutdownMedia_m();

    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnable(this, &TestAgent::Shutdown_s));
  }

  uint32_t GetRemoteSSRC() {
    uint32_t res = 0;
    audio_conduit_->GetRemoteSSRC(&res);
    return res;
  }

  uint32_t GetLocalSSRC() {
    uint32_t res = 0;
    audio_conduit_->GetLocalSSRC(&res);
    return res;
  }

  int GetAudioRtpCountSent() {
    return audio_pipeline_->rtp_packets_sent();
  }

  int GetAudioRtpCountReceived() {
    return audio_pipeline_->rtp_packets_received();
  }

  int GetAudioRtcpCountSent() {
    return audio_pipeline_->rtcp_packets_sent();
  }

  int GetAudioRtcpCountReceived() {
    return audio_pipeline_->rtcp_packets_received();
  }

 protected:
  mozilla::AudioCodecConfig audio_config_;
  mozilla::RefPtr<mozilla::MediaSessionConduit> audio_conduit_;
  nsRefPtr<DOMMediaStream> audio_;
  
  
  
  mozilla::RefPtr<mozilla::MediaPipeline> audio_pipeline_;
  TransportInfo audio_rtp_transport_;
  TransportInfo audio_rtcp_transport_;
  TransportInfo bundle_transport_;
};

class TestAgentSend : public TestAgent {
 public:
  TestAgentSend() : use_bundle_(false) {}

  virtual void CreatePipelines_s(bool aIsRtcpMux) {
    audio_ = new Fake_DOMMediaStream(new Fake_AudioStreamSource());

    mozilla::MediaConduitErrorCode err =
        static_cast<mozilla::AudioSessionConduit *>(audio_conduit_.get())->
        ConfigureSendMediaCodec(&audio_config_);
    EXPECT_EQ(mozilla::kMediaConduitNoError, err);

    std::string test_pc("PC");

    if (aIsRtcpMux) {
      ASSERT_FALSE(audio_rtcp_transport_.flow_);
    }

    RefPtr<TransportFlow> rtp(audio_rtp_transport_.flow_);
    RefPtr<TransportFlow> rtcp(audio_rtcp_transport_.flow_);

    if (use_bundle_) {
      rtp = bundle_transport_.flow_;
      rtcp = nullptr;
    }

    audio_pipeline_ = new mozilla::MediaPipelineTransmit(
        test_pc,
        nullptr,
        test_utils->sts_target(),
        audio_,
        1,
        1,
        false,
        audio_conduit_,
        rtp,
        rtcp);

    audio_pipeline_->Init();
  }

  void SetUsingBundle(bool use_bundle) {
    use_bundle_ = use_bundle;
  }

 private:
  bool use_bundle_;
};


class TestAgentReceive : public TestAgent {
 public:
  virtual void CreatePipelines_s(bool aIsRtcpMux) {
    mozilla::SourceMediaStream *audio = new Fake_SourceMediaStream();
    audio->SetPullEnabled(true);

    mozilla::AudioSegment* segment= new mozilla::AudioSegment();
    audio->AddTrack(0, 100, 0, segment);
    audio->AdvanceKnownTracksTime(mozilla::STREAM_TIME_MAX);

    audio_ = new Fake_DOMMediaStream(audio);

    std::vector<mozilla::AudioCodecConfig *> codecs;
    codecs.push_back(&audio_config_);

    mozilla::MediaConduitErrorCode err =
        static_cast<mozilla::AudioSessionConduit *>(audio_conduit_.get())->
        ConfigureRecvMediaCodecs(codecs);
    EXPECT_EQ(mozilla::kMediaConduitNoError, err);

    std::string test_pc("PC");

    if (aIsRtcpMux) {
      ASSERT_FALSE(audio_rtcp_transport_.flow_);
    }

    
    RefPtr<TransportFlow> dummy;
    RefPtr<TransportFlow> bundle_transport;
    if (bundle_filter_) {
      bundle_transport = bundle_transport_.flow_;
      bundle_filter_->AddLocalSSRC(GetLocalSSRC());
    }

    audio_pipeline_ = new mozilla::MediaPipelineReceiveAudio(
        test_pc,
        nullptr,
        test_utils->sts_target(),
        audio_->GetStream(), 1, 1,
        static_cast<mozilla::AudioSessionConduit *>(audio_conduit_.get()),
        audio_rtp_transport_.flow_,
        audio_rtcp_transport_.flow_,
        bundle_transport,
        dummy,
        bundle_filter_);

    audio_pipeline_->Init();
  }

  void SetBundleFilter(nsAutoPtr<MediaPipelineFilter> filter) {
    bundle_filter_ = filter;
  }

  void SetUsingBundle_s(bool decision) {
    audio_pipeline_->SetUsingBundle_s(decision);
  }

  void UpdateFilterFromRemoteDescription_s(
      nsAutoPtr<MediaPipelineFilter> filter) {
    audio_pipeline_->UpdateFilterFromRemoteDescription_s(filter);
  }

 private:
  nsAutoPtr<MediaPipelineFilter> bundle_filter_;
};


class MediaPipelineTest : public ::testing::Test {
 public:
  ~MediaPipelineTest() {
    p1_.Stop();
    p2_.Stop();
    p1_.Shutdown();
    p2_.Shutdown();
  }

  
  void InitTransports(bool aIsRtcpMux) {
    
    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnableNM(&TestAgent::ConnectRtp, &p2_, &p1_));

    
    if(!aIsRtcpMux) {
      
      mozilla::SyncRunnable::DispatchToThread(
        test_utils->sts_target(),
        WrapRunnableNM(&TestAgent::ConnectRtcp, &p2_, &p1_));
    }

    
    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnableNM(&TestAgent::ConnectBundle, &p2_, &p1_));
  }

  
  void TestAudioSend(bool aIsRtcpMux,
                     bool bundle = false,
                     nsAutoPtr<MediaPipelineFilter> localFilter =
                        nsAutoPtr<MediaPipelineFilter>(nullptr),
                     nsAutoPtr<MediaPipelineFilter> remoteFilter =
                        nsAutoPtr<MediaPipelineFilter>(nullptr),
                     unsigned int ms_until_answer = 500,
                     unsigned int ms_of_traffic_after_answer = 10000) {

    
    
    ASSERT_FALSE(!aIsRtcpMux && bundle);

    p1_.SetUsingBundle(bundle);
    p2_.SetBundleFilter(localFilter);

    
    InitTransports(aIsRtcpMux);

    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnable(&p1_, &TestAgent::CreatePipelines_s, aIsRtcpMux));

    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnable(&p2_, &TestAgent::CreatePipelines_s, aIsRtcpMux));

    p2_.Start();
    p1_.Start();

    
    PR_Sleep(ms_until_answer);

    mozilla::SyncRunnable::DispatchToThread(
      test_utils->sts_target(),
      WrapRunnable(&p2_, &TestAgentReceive::SetUsingBundle_s, bundle));

    if (bundle) {
      
      if (!remoteFilter) {
        remoteFilter = new MediaPipelineFilter;
        
        remoteFilter->AddRemoteSSRC(p1_.GetLocalSSRC());
      }

      mozilla::SyncRunnable::DispatchToThread(
          test_utils->sts_target(),
          WrapRunnable(&p2_,
                       &TestAgentReceive::UpdateFilterFromRemoteDescription_s,
                       remoteFilter));
    }


    
    PR_Sleep(ms_of_traffic_after_answer);

    p1_.Stop();
    p2_.Stop();

    
    PR_Sleep(100);

    p1_.Shutdown();
    p2_.Shutdown();

    if (!bundle) {
      
      ASSERT_GE(p1_.GetAudioRtpCountSent(), 40);
      ASSERT_EQ(p1_.GetAudioRtpCountReceived(), p2_.GetAudioRtpCountSent());
      ASSERT_EQ(p1_.GetAudioRtpCountSent(), p2_.GetAudioRtpCountReceived());

      
      
      ASSERT_LE(p2_.GetAudioRtcpCountReceived(), p1_.GetAudioRtcpCountSent());
      ASSERT_GE(p2_.GetAudioRtcpCountReceived() + 1, p1_.GetAudioRtcpCountSent());
    }

  }

  void TestAudioReceiverOffersBundle(bool bundle_accepted,
      nsAutoPtr<MediaPipelineFilter> localFilter,
      nsAutoPtr<MediaPipelineFilter> remoteFilter =
          nsAutoPtr<MediaPipelineFilter>(nullptr),
      unsigned int ms_until_answer = 500,
      unsigned int ms_of_traffic_after_answer = 10000) {
    TestAudioSend(true,
                  bundle_accepted,
                  localFilter,
                  remoteFilter,
                  ms_until_answer,
                  ms_of_traffic_after_answer);
  }
protected:
  TestAgentSend p1_;
  TestAgentReceive p2_;
};

class MediaPipelineFilterTest : public ::testing::Test {
  public:
    bool Filter(MediaPipelineFilter& filter,
                int32_t correlator,
                uint32_t ssrc,
                uint8_t payload_type) {

      webrtc::RTPHeader header;
      header.ssrc = ssrc;
      header.payloadType = payload_type;
      return filter.Filter(header, correlator);
    }
};

TEST_F(MediaPipelineFilterTest, TestConstruct) {
  MediaPipelineFilter filter;
}

TEST_F(MediaPipelineFilterTest, TestDefault) {
  MediaPipelineFilter filter;
  ASSERT_FALSE(Filter(filter, 0, 233, 110));
}

TEST_F(MediaPipelineFilterTest, TestSSRCFilter) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(555);
  ASSERT_TRUE(Filter(filter, 0, 555, 110));
  ASSERT_FALSE(Filter(filter, 0, 556, 110));
}

#define SSRC(ssrc) \
  ((ssrc >> 24) & 0xFF), \
  ((ssrc >> 16) & 0xFF), \
  ((ssrc >> 8 ) & 0xFF), \
  (ssrc         & 0xFF)

#define REPORT_FRAGMENT(ssrc) \
  SSRC(ssrc), \
  0,0,0,0, \
  0,0,0,0, \
  0,0,0,0, \
  0,0,0,0, \
  0,0,0,0

#define RTCP_TYPEINFO(num_rrs, type, size) \
  0x80 + num_rrs, type, 0, size

const unsigned char rtcp_rr_s16[] = {
  
  RTCP_TYPEINFO(0, MediaPipelineFilter::RECEIVER_REPORT_T, 1),
  SSRC(16)
};

const unsigned char rtcp_rr_s16_r17[] = {
  
  RTCP_TYPEINFO(1, MediaPipelineFilter::RECEIVER_REPORT_T, 7),
  SSRC(16),
  REPORT_FRAGMENT(17)
};

const unsigned char rtcp_rr_s16_r17_18[] = {
  
  RTCP_TYPEINFO(2, MediaPipelineFilter::RECEIVER_REPORT_T, 13),
  SSRC(16),
  REPORT_FRAGMENT(17),
  REPORT_FRAGMENT(18)
};

const unsigned char rtcp_sr_s16[] = {
  
  RTCP_TYPEINFO(0, MediaPipelineFilter::SENDER_REPORT_T, 6),
  REPORT_FRAGMENT(16)
};

const unsigned char rtcp_sr_s16_r17[] = {
  
  RTCP_TYPEINFO(1, MediaPipelineFilter::SENDER_REPORT_T, 12),
  REPORT_FRAGMENT(16),
  REPORT_FRAGMENT(17)
};

const unsigned char rtcp_sr_s16_r17_18[] = {
  
  RTCP_TYPEINFO(2, MediaPipelineFilter::SENDER_REPORT_T, 18),
  REPORT_FRAGMENT(16),
  REPORT_FRAGMENT(17),
  REPORT_FRAGMENT(18)
};

const unsigned char unknown_type[] = {
  RTCP_TYPEINFO(1, 222, 0)
};

TEST_F(MediaPipelineFilterTest, TestEmptyFilterReport0) {
  MediaPipelineFilter filter;
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_sr_s16, sizeof(rtcp_sr_s16)));
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_rr_s16, sizeof(rtcp_rr_s16)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport0) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16, sizeof(rtcp_sr_s16)));
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_rr_s16, sizeof(rtcp_rr_s16)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport0SSRCTruncated) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  const unsigned char data[] = {
    RTCP_TYPEINFO(0, MediaPipelineFilter::RECEIVER_REPORT_T, 1),
    0,0,0
  };
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(data, sizeof(data)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport0PTTruncated) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  const unsigned char data[] = {0x80};
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(data, sizeof(data)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport0CountTruncated) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  const unsigned char data[] = {};
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(data, sizeof(data)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport1BothMatch) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  filter.AddLocalSSRC(17);
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16_r17, sizeof(rtcp_sr_s16_r17)));
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_rr_s16_r17, sizeof(rtcp_rr_s16_r17)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport1SSRCTruncated) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  filter.AddLocalSSRC(17);
  const unsigned char rr[] = {
    RTCP_TYPEINFO(1, MediaPipelineFilter::RECEIVER_REPORT_T, 7),
    SSRC(16),
    0,0,0
  };
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rr, sizeof(rr)));
  const unsigned char sr[] = {
    RTCP_TYPEINFO(1, MediaPipelineFilter::RECEIVER_REPORT_T, 12),
    REPORT_FRAGMENT(16),
    0,0,0
  };
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(sr, sizeof(rr)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport1BigSSRC) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(0x01020304);
  filter.AddLocalSSRC(0x11121314);
  const unsigned char rr[] = {
    RTCP_TYPEINFO(1, MediaPipelineFilter::RECEIVER_REPORT_T, 7),
    SSRC(0x01020304),
    REPORT_FRAGMENT(0x11121314)
  };
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rr, sizeof(rr)));
  const unsigned char sr[] = {
    RTCP_TYPEINFO(1, MediaPipelineFilter::RECEIVER_REPORT_T, 12),
    SSRC(0x01020304),
    REPORT_FRAGMENT(0x11121314)
  };
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(sr, sizeof(rr)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport1LocalMatch) {
  MediaPipelineFilter filter;
  filter.AddLocalSSRC(17);
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16_r17, sizeof(rtcp_sr_s16_r17)));
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_rr_s16_r17, sizeof(rtcp_rr_s16_r17)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport1Inconsistent) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  
  
  
  
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_sr_s16_r17, sizeof(rtcp_sr_s16_r17)));
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_rr_s16_r17, sizeof(rtcp_rr_s16_r17)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport1NeitherMatch) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(17);
  filter.AddLocalSSRC(18);
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16_r17, sizeof(rtcp_sr_s16_r17)));
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_rr_s16_r17, sizeof(rtcp_rr_s16_r17)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport2AllMatch) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  filter.AddLocalSSRC(17);
  filter.AddLocalSSRC(18);
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16_r17_18,
                              sizeof(rtcp_sr_s16_r17_18)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport2LocalMatch) {
  MediaPipelineFilter filter;
  filter.AddLocalSSRC(17);
  filter.AddLocalSSRC(18);
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16_r17_18,
                              sizeof(rtcp_sr_s16_r17_18)));
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_rr_s16_r17_18,
                              sizeof(rtcp_rr_s16_r17_18)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport2Inconsistent101) {
  MediaPipelineFilter filter;
  filter.AddRemoteSSRC(16);
  filter.AddLocalSSRC(18);
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_sr_s16_r17_18,
                              sizeof(rtcp_sr_s16_r17_18)));
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_rr_s16_r17_18,
                              sizeof(rtcp_rr_s16_r17_18)));
}

TEST_F(MediaPipelineFilterTest, TestFilterReport2Inconsistent001) {
  MediaPipelineFilter filter;
  filter.AddLocalSSRC(18);
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_sr_s16_r17_18,
                              sizeof(rtcp_sr_s16_r17_18)));
  ASSERT_EQ(MediaPipelineFilter::FAIL,
            filter.FilterRTCP(rtcp_rr_s16_r17_18,
                              sizeof(rtcp_rr_s16_r17_18)));
}

TEST_F(MediaPipelineFilterTest, TestFilterUnknownRTCPType) {
  MediaPipelineFilter filter;
  filter.AddLocalSSRC(18);
  ASSERT_EQ(MediaPipelineFilter::UNSUPPORTED,
            filter.FilterRTCP(unknown_type, sizeof(unknown_type)));
}

TEST_F(MediaPipelineFilterTest, TestCorrelatorFilter) {
  MediaPipelineFilter filter;
  filter.SetCorrelator(7777);
  ASSERT_TRUE(Filter(filter, 7777, 16, 110));
  ASSERT_FALSE(Filter(filter, 7778, 17, 110));
  
  ASSERT_TRUE(Filter(filter, 0, 16, 110));
  ASSERT_FALSE(Filter(filter, 0, 17, 110));

  
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16, sizeof(rtcp_sr_s16)));
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_rr_s16, sizeof(rtcp_rr_s16)));
}

TEST_F(MediaPipelineFilterTest, TestPayloadTypeFilter) {
  MediaPipelineFilter filter;
  filter.AddUniquePT(110);
  ASSERT_TRUE(Filter(filter, 0, 555, 110));
  ASSERT_FALSE(Filter(filter, 0, 556, 111));
}

TEST_F(MediaPipelineFilterTest, TestPayloadTypeFilterSSRCUpdate) {
  MediaPipelineFilter filter;
  filter.AddUniquePT(110);
  ASSERT_TRUE(Filter(filter, 0, 16, 110));

  
  ASSERT_EQ(MediaPipelineFilter::PASS,
            filter.FilterRTCP(rtcp_sr_s16, sizeof(rtcp_sr_s16)));
}

TEST_F(MediaPipelineFilterTest, TestAnswerAddsSSRCs) {
  MediaPipelineFilter filter;
  filter.SetCorrelator(7777);
  ASSERT_TRUE(Filter(filter, 7777, 555, 110));
  ASSERT_FALSE(Filter(filter, 7778, 556, 110));
  
  ASSERT_TRUE(Filter(filter, 0, 555, 110));
  ASSERT_FALSE(Filter(filter, 0, 556, 110));

  
  
  MediaPipelineFilter filter2;
  filter2.AddRemoteSSRC(555);
  filter2.AddRemoteSSRC(556);
  filter2.AddRemoteSSRC(557);

  filter.IncorporateRemoteDescription(filter2);

  
  ASSERT_TRUE(Filter(filter, 0, 555, 110));

  
  ASSERT_TRUE(Filter(filter, 0, 556, 110));
  ASSERT_TRUE(Filter(filter, 0, 557, 110));

  
  ASSERT_TRUE(Filter(filter, 7777, 558, 110));
}

TEST_F(MediaPipelineFilterTest, TestSSRCMovedWithSDP) {
  MediaPipelineFilter filter;
  filter.SetCorrelator(7777);
  filter.AddUniquePT(111);
  ASSERT_TRUE(Filter(filter, 7777, 555, 110));

  MediaPipelineFilter filter2;
  filter2.AddRemoteSSRC(556);

  filter.IncorporateRemoteDescription(filter2);

  
  ASSERT_FALSE(Filter(filter, 0, 555, 110));

  
  ASSERT_TRUE(Filter(filter, 0, 556, 110));

  
  ASSERT_TRUE(Filter(filter, 7777, 558, 110));

  
  ASSERT_TRUE(Filter(filter, 0, 559, 111));
}

TEST_F(MediaPipelineFilterTest, TestSSRCMovedWithCorrelator) {
  MediaPipelineFilter filter;
  filter.SetCorrelator(7777);
  ASSERT_TRUE(Filter(filter, 7777, 555, 110));
  ASSERT_TRUE(Filter(filter, 0, 555, 110));
  ASSERT_FALSE(Filter(filter, 7778, 555, 110));
  ASSERT_FALSE(Filter(filter, 0, 555, 110));
}

TEST_F(MediaPipelineFilterTest, TestRemoteSDPNoSSRCs) {
  
  
  
  MediaPipelineFilter filter;
  filter.SetCorrelator(7777);
  filter.AddUniquePT(111);
  ASSERT_TRUE(Filter(filter, 7777, 555, 110));

  MediaPipelineFilter filter2;

  filter.IncorporateRemoteDescription(filter2);

  
  ASSERT_TRUE(Filter(filter, 7777, 555, 110));
}

TEST_F(MediaPipelineTest, TestAudioSendNoMux) {
  TestAudioSend(false);
}

TEST_F(MediaPipelineTest, TestAudioSendMux) {
  TestAudioSend(true);
}

TEST_F(MediaPipelineTest, TestAudioSendBundleOfferedAndDeclined) {
  nsAutoPtr<MediaPipelineFilter> filter(new MediaPipelineFilter);
  TestAudioReceiverOffersBundle(false, filter);
}

TEST_F(MediaPipelineTest, TestAudioSendBundleOfferedAndAccepted) {
  nsAutoPtr<MediaPipelineFilter> filter(new MediaPipelineFilter);
  
  
  
  
  TestAudioReceiverOffersBundle(true,
                                filter,
  
  
                                nsAutoPtr<MediaPipelineFilter>(),
                                10000,
                                10000);

  
  ASSERT_GT(p1_.GetAudioRtpCountSent(), p2_.GetAudioRtpCountReceived());
  ASSERT_GT(p2_.GetAudioRtpCountReceived(), 40);
  ASSERT_GT(p1_.GetAudioRtcpCountSent(), 1);
  ASSERT_GT(p1_.GetAudioRtcpCountSent(), p2_.GetAudioRtcpCountReceived());
  ASSERT_GT(p2_.GetAudioRtcpCountReceived(), 0);
}

TEST_F(MediaPipelineTest, TestAudioSendBundleOfferedAndAcceptedEmptyFilter) {
  nsAutoPtr<MediaPipelineFilter> filter(new MediaPipelineFilter);
  nsAutoPtr<MediaPipelineFilter> bad_answer_filter(new MediaPipelineFilter);
  TestAudioReceiverOffersBundle(true, filter, bad_answer_filter);
  
  ASSERT_EQ(0, p2_.GetAudioRtpCountReceived());
  ASSERT_EQ(0, p2_.GetAudioRtcpCountReceived());
}

}  


int main(int argc, char **argv) {
  test_utils = new MtransportTestUtils();
  
  NSS_NoDB_Init(nullptr);
  NSS_SetDomesticPolicy();
  ::testing::InitGoogleTest(&argc, argv);

  int rv = RUN_ALL_TESTS();
  delete test_utils;
  return rv;
}



