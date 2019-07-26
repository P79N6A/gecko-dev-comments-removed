









#include "../acm2/acm_common_defs.h"
#include "gtest/gtest.h"
#include "audio_coding_module.h"
#include "PCMFile.h"
#include "module_common_types.h"
#include "scoped_ptr.h"
#include "typedefs.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace webrtc {

class DualStreamTest :
    public AudioPacketizationCallback,
    public ::testing::Test {
 protected:
  DualStreamTest();
  ~DualStreamTest();

  int32_t SendData(FrameType frameType, uint8_t payload_type,
                   uint32_t timestamp, const uint8_t* payload_data,
                   uint16_t payload_size,
                   const RTPFragmentationHeader* fragmentation);

  void Perform(bool start_in_sync, int num_channels_input);

  void InitializeSender(int frame_size_primary_samples,
                        int num_channels_primary, int sampling_rate);

  void PopulateCodecInstances(int frame_size_primary_ms,
                              int num_channels_primary, int sampling_rate);

  void Validate(bool start_in_sync, int tolerance);
  bool EqualTimestamp(int stream, int position);
  int EqualPayloadLength(int stream, int position);
  bool EqualPayloadData(int stream, int position);

  static const int kMaxNumStoredPayloads = 2;

  enum {
    kPrimary = 0,
    kSecondary,
    kMaxNumStreams
  };

  scoped_ptr<AudioCodingModule> acm_dual_stream_;
  scoped_ptr<AudioCodingModule> acm_ref_primary_;
  scoped_ptr<AudioCodingModule> acm_ref_secondary_;

  CodecInst primary_encoder_;
  CodecInst secondary_encoder_;

  CodecInst red_encoder_;

  int payload_ref_is_stored_[kMaxNumStreams][kMaxNumStoredPayloads];
  int payload_dual_is_stored_[kMaxNumStreams][kMaxNumStoredPayloads];

  uint32_t timestamp_ref_[kMaxNumStreams][kMaxNumStoredPayloads];
  uint32_t timestamp_dual_[kMaxNumStreams][kMaxNumStoredPayloads];

  int payload_len_ref_[kMaxNumStreams][kMaxNumStoredPayloads];
  int payload_len_dual_[kMaxNumStreams][kMaxNumStoredPayloads];

  uint8_t payload_data_ref_[kMaxNumStreams][MAX_PAYLOAD_SIZE_BYTE
      * kMaxNumStoredPayloads];
  uint8_t payload_data_dual_[kMaxNumStreams][MAX_PAYLOAD_SIZE_BYTE
      * kMaxNumStoredPayloads];
  int num_received_payloads_dual_[kMaxNumStreams];
  int num_received_payloads_ref_[kMaxNumStreams];

  int num_compared_payloads_[kMaxNumStreams];
  uint32_t last_timestamp_[kMaxNumStreams];
  bool received_payload_[kMaxNumStreams];
};

DualStreamTest::DualStreamTest()
    : acm_dual_stream_(AudioCodingModule::Create(0)),
      acm_ref_primary_(AudioCodingModule::Create(1)),
      acm_ref_secondary_(AudioCodingModule::Create(2)),
      payload_ref_is_stored_(),
      payload_dual_is_stored_(),
      timestamp_ref_(),
      num_received_payloads_dual_(),
      num_received_payloads_ref_(),
      num_compared_payloads_(),
      last_timestamp_(),
      received_payload_() {
}

DualStreamTest::~DualStreamTest() {
}

void DualStreamTest::PopulateCodecInstances(int frame_size_primary_ms,
                                            int num_channels_primary,
                                            int sampling_rate) {
  CodecInst my_codec;

  
  primary_encoder_.pltype = -1;
  secondary_encoder_.pltype = -1;
  red_encoder_.pltype = -1;

  for (int n = 0; n < AudioCodingModule::NumberOfCodecs(); n++) {
    AudioCodingModule::Codec(n, &my_codec);
    if (strcmp(my_codec.plname, "ISAC") == 0
        && my_codec.plfreq == sampling_rate) {
      my_codec.rate = 32000;
      my_codec.pacsize = 30 * sampling_rate / 1000;
      memcpy(&secondary_encoder_, &my_codec, sizeof(my_codec));
    } else if (strcmp(my_codec.plname, "L16") == 0
        && my_codec.channels == num_channels_primary
        && my_codec.plfreq == sampling_rate) {
      my_codec.pacsize = frame_size_primary_ms * sampling_rate / 1000;
      memcpy(&primary_encoder_, &my_codec, sizeof(my_codec));
    } else if (strcmp(my_codec.plname, "red") == 0) {
      memcpy(&red_encoder_, &my_codec, sizeof(my_codec));
    }
  }

  ASSERT_GE(primary_encoder_.pltype, 0);
  ASSERT_GE(secondary_encoder_.pltype, 0);
  ASSERT_GE(red_encoder_.pltype, 0);
}

void DualStreamTest::InitializeSender(int frame_size_primary_samples,
                                      int num_channels_primary,
                                      int sampling_rate) {
  ASSERT_TRUE(acm_dual_stream_.get() != NULL);
  ASSERT_TRUE(acm_ref_primary_.get() != NULL);
  ASSERT_TRUE(acm_ref_secondary_.get() != NULL);

  ASSERT_EQ(0, acm_dual_stream_->InitializeSender());
  ASSERT_EQ(0, acm_ref_primary_->InitializeSender());
  ASSERT_EQ(0, acm_ref_secondary_->InitializeSender());

  PopulateCodecInstances(frame_size_primary_samples, num_channels_primary,
                         sampling_rate);

  ASSERT_EQ(0, acm_ref_primary_->RegisterSendCodec(primary_encoder_));
  ASSERT_EQ(0, acm_ref_secondary_->RegisterSendCodec(secondary_encoder_));
  ASSERT_EQ(0, acm_dual_stream_->RegisterSendCodec(primary_encoder_));
  ASSERT_EQ(0,
            acm_dual_stream_->RegisterSecondarySendCodec(secondary_encoder_));

  ASSERT_EQ(0, acm_ref_primary_->RegisterTransportCallback(this));
  ASSERT_EQ(0, acm_ref_secondary_->RegisterTransportCallback(this));
  ASSERT_EQ(0, acm_dual_stream_->RegisterTransportCallback(this));
}

void DualStreamTest::Perform(bool start_in_sync, int num_channels_input) {
  PCMFile pcm_file;
  std::string file_name = test::ResourcePath(
      (num_channels_input == 1) ?
          "audio_coding/testfile32kHz" : "audio_coding/teststereo32kHz",
      "pcm");
  pcm_file.Open(file_name, 32000, "rb");
  pcm_file.ReadStereo(num_channels_input == 2);
  AudioFrame audio_frame;

  int tolerance = 0;
  if (num_channels_input == 2 && primary_encoder_.channels == 2
      && secondary_encoder_.channels == 1) {
    tolerance = 12;
  }

  if (!start_in_sync) {
    pcm_file.Read10MsData(audio_frame);
    
    acm_dual_stream_->UnregisterSecondarySendCodec();
    EXPECT_EQ(0, acm_dual_stream_->Add10MsData(audio_frame));
    EXPECT_EQ(0, acm_ref_primary_->Add10MsData(audio_frame));
    ASSERT_EQ(0,
              acm_dual_stream_->RegisterSecondarySendCodec(secondary_encoder_));
  }

  const int kNumFramesToProcess = 100;
  int frame_cntr = 0;
  while (!pcm_file.EndOfFile() && frame_cntr < kNumFramesToProcess) {
    pcm_file.Read10MsData(audio_frame);
    frame_cntr++;
    EXPECT_EQ(0, acm_dual_stream_->Add10MsData(audio_frame));
    EXPECT_EQ(0, acm_ref_primary_->Add10MsData(audio_frame));
    EXPECT_EQ(0, acm_ref_secondary_->Add10MsData(audio_frame));

    EXPECT_GE(acm_dual_stream_->Process(), 0);
    EXPECT_GE(acm_ref_primary_->Process(), 0);
    EXPECT_GE(acm_ref_secondary_->Process(), 0);

    if (start_in_sync || frame_cntr > 7) {
      
      
      
      Validate(start_in_sync, tolerance);
    } else {
      
      
      memset(payload_ref_is_stored_, 0, sizeof(payload_ref_is_stored_));
      memset(payload_dual_is_stored_, 0, sizeof(payload_dual_is_stored_));
    }
  }
  pcm_file.Close();

  
  
  
  
  EXPECT_EQ(num_received_payloads_ref_[kPrimary],
            num_received_payloads_dual_[kPrimary]);
  EXPECT_TRUE(
      num_received_payloads_ref_[kSecondary]
          == num_received_payloads_dual_[kSecondary]
          || num_received_payloads_ref_[kSecondary]
              == (num_received_payloads_dual_[kSecondary] + 1));

  
  if (start_in_sync) {
    EXPECT_EQ(num_received_payloads_dual_[kPrimary],
              num_compared_payloads_[kPrimary]);
    EXPECT_EQ(num_received_payloads_dual_[kSecondary],
              num_compared_payloads_[kSecondary]);
  } else {
    
    
    EXPECT_GE(num_compared_payloads_[kPrimary],
              num_received_payloads_dual_[kPrimary] - 4);
    EXPECT_GE(num_compared_payloads_[kSecondary],
              num_received_payloads_dual_[kSecondary] - 4);
  }
}

bool DualStreamTest::EqualTimestamp(int stream_index, int position) {
  if (timestamp_dual_[stream_index][position]
      != timestamp_ref_[stream_index][position]) {
    return false;
  }
  return true;
}

int DualStreamTest::EqualPayloadLength(int stream_index, int position) {
  return abs(
      payload_len_dual_[stream_index][position]
          - payload_len_ref_[stream_index][position]);
}

bool DualStreamTest::EqualPayloadData(int stream_index, int position) {
  assert(
      payload_len_dual_[stream_index][position]
          == payload_len_ref_[stream_index][position]);
  int offset = position * MAX_PAYLOAD_SIZE_BYTE;
  for (int n = 0; n < payload_len_dual_[stream_index][position]; n++) {
    if (payload_data_dual_[stream_index][offset + n]
        != payload_data_ref_[stream_index][offset + n]) {
      return false;
    }
  }
  return true;
}

void DualStreamTest::Validate(bool start_in_sync, int tolerance) {
  for (int stream_index = 0; stream_index < kMaxNumStreams; stream_index++) {
    int my_tolerance = stream_index == kPrimary ? 0 : tolerance;
    for (int position = 0; position < kMaxNumStoredPayloads; position++) {
      if (payload_ref_is_stored_[stream_index][position] == 1
          && payload_dual_is_stored_[stream_index][position] == 1) {
        
        if (start_in_sync || stream_index == 0)
          EXPECT_TRUE(EqualTimestamp(stream_index, position));
        EXPECT_LE(EqualPayloadLength(stream_index, position), my_tolerance);
        if (my_tolerance == 0)
          EXPECT_TRUE(EqualPayloadData(stream_index, position));
        num_compared_payloads_[stream_index]++;
        payload_ref_is_stored_[stream_index][position] = 0;
        payload_dual_is_stored_[stream_index][position] = 0;
      }
    }
  }
}

int32_t DualStreamTest::SendData(FrameType frameType, uint8_t payload_type,
                                 uint32_t timestamp,
                                 const uint8_t* payload_data,
                                 uint16_t payload_size,
                                 const RTPFragmentationHeader* fragmentation) {
  int position;
  int stream_index;

  if (payload_type == red_encoder_.pltype) {
    if (fragmentation == NULL) {
      assert(false);
      return -1;
    }
    
    
    
    for (int n = fragmentation->fragmentationVectorSize - 1; n >= 0; --n) {
      if (fragmentation->fragmentationPlType[n] == primary_encoder_.pltype) {
        
        stream_index = kPrimary;
      } else if (fragmentation->fragmentationPlType[n]
          == secondary_encoder_.pltype) {
        
        stream_index = kSecondary;
      } else {
        assert(false);
        return -1;
      }
      num_received_payloads_dual_[stream_index]++;
      if (payload_dual_is_stored_[stream_index][0] == 0) {
        position = 0;
      } else if (payload_dual_is_stored_[stream_index][1] == 0) {
        position = 1;
      } else {
        assert(false);
        return -1;
      }
      timestamp_dual_[stream_index][position] = timestamp
          - fragmentation->fragmentationTimeDiff[n];
      payload_len_dual_[stream_index][position] = fragmentation
          ->fragmentationLength[n];
      memcpy(
          &payload_data_dual_[stream_index][position * MAX_PAYLOAD_SIZE_BYTE],
          &payload_data[fragmentation->fragmentationOffset[n]],
          fragmentation->fragmentationLength[n]);
      payload_dual_is_stored_[stream_index][position] = 1;
      
      if (received_payload_[stream_index]) {
        int t = timestamp_dual_[stream_index][position]
            - last_timestamp_[stream_index];
        if ((stream_index == kPrimary) && (t != primary_encoder_.pacsize)) {
          assert(false);
          return -1;
        }
        if ((stream_index == kSecondary) && (t != secondary_encoder_.pacsize)) {
          assert(false);
          return -1;
        }
      } else {
        received_payload_[stream_index] = true;
      }
      last_timestamp_[stream_index] = timestamp_dual_[stream_index][position];
    }
  } else {
    if (fragmentation != NULL) {
      assert(false);
      return -1;
    }
    if (payload_type == primary_encoder_.pltype) {
      stream_index = kPrimary;
    } else if (payload_type == secondary_encoder_.pltype) {
      stream_index = kSecondary;
    } else {
      assert(false);
      return -1;
    }
    num_received_payloads_ref_[stream_index]++;
    if (payload_ref_is_stored_[stream_index][0] == 0) {
      position = 0;
    } else if (payload_ref_is_stored_[stream_index][1] == 0) {
      position = 1;
    } else {
      assert(false);
      return -1;
    }
    timestamp_ref_[stream_index][position] = timestamp;
    payload_len_ref_[stream_index][position] = payload_size;
    memcpy(&payload_data_ref_[stream_index][position * MAX_PAYLOAD_SIZE_BYTE],
           payload_data, payload_size);
    payload_ref_is_stored_[stream_index][position] = 1;
  }
  return 0;
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncMonoInputMonoPrimaryWb20Ms)) {
  InitializeSender(20, 1, 16000);
  Perform(true, 1);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncMonoInput_StereoPrimaryWb20Ms)) {
  InitializeSender(20, 2, 16000);
  Perform(true, 1);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncMonoInputMonoPrimarySwb20Ms)) {
  InitializeSender(20, 1, 32000);
  Perform(true, 1);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncMonoInputStereoPrimarySwb20Ms)) {
  InitializeSender(20, 2, 32000);
  Perform(true, 1);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncMonoInputMonoPrimaryWb40Ms)) {
  InitializeSender(40, 1, 16000);
  Perform(true, 1);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncMonoInputStereoPrimaryWb40Ms)) {
  InitializeSender(40, 2, 16000);
  Perform(true, 1);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncStereoInputMonoPrimaryWb20Ms)) {
  InitializeSender(20, 1, 16000);
  Perform(true, 2);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncStereoInputStereoPrimaryWb20Ms)) {
  InitializeSender(20, 2, 16000);
  Perform(true, 2);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncStereoInputMonoPrimarySwb20Ms)) {
  InitializeSender(20, 1, 32000);
  Perform(true, 2);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncStereoInputStereoPrimarySwb20Ms)) {
  InitializeSender(20, 2, 32000);
  Perform(true, 2);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncStereoInputMonoPrimaryWb40Ms)) {
  InitializeSender(40, 1, 16000);
  Perform(true, 2);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactSyncStereoInputStereoPrimaryWb40Ms)) {
  InitializeSender(40, 2, 16000);
  Perform(true, 2);
}



TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactAsyncMonoInputMonoPrimaryWb20Ms)) {
  InitializeSender(20, 1, 16000);
  Perform(false, 1);
}


TEST_F(DualStreamTest,
       DISABLED_ON_ANDROID(BitExactAsyncMonoInputMonoPrimaryWb40Ms)) {
  InitializeSender(40, 1, 16000);
  Perform(false, 1);
}

TEST_F(DualStreamTest, DISABLED_ON_ANDROID(Api)) {
  PopulateCodecInstances(20, 1, 16000);
  CodecInst my_codec;
  ASSERT_EQ(0, acm_dual_stream_->InitializeSender());
  ASSERT_EQ(-1, acm_dual_stream_->SecondarySendCodec(&my_codec));

  
  ASSERT_EQ(-1,
      acm_dual_stream_->RegisterSecondarySendCodec(secondary_encoder_));
  ASSERT_EQ(-1, acm_dual_stream_->SecondarySendCodec(&my_codec));

  ASSERT_EQ(0, acm_dual_stream_->RegisterSendCodec(primary_encoder_));

  ASSERT_EQ(0, acm_dual_stream_->SetVAD(true, true, VADNormal));

  
  bool vad_status;
  bool dtx_status;
  ACMVADMode vad_mode;
  EXPECT_EQ(0, acm_dual_stream_->VAD(&vad_status, &dtx_status, &vad_mode));
  EXPECT_TRUE(vad_status);
  EXPECT_TRUE(dtx_status);
  EXPECT_EQ(VADNormal, vad_mode);

  ASSERT_EQ(0,
      acm_dual_stream_->RegisterSecondarySendCodec(secondary_encoder_));

  ASSERT_EQ(0, acm_dual_stream_->SecondarySendCodec(&my_codec));
  ASSERT_EQ(0, memcmp(&my_codec, &secondary_encoder_, sizeof(my_codec)));

  
  EXPECT_EQ(0, acm_dual_stream_->VAD(&vad_status, &dtx_status, &vad_mode));
  EXPECT_FALSE(vad_status);
  EXPECT_FALSE(dtx_status);

  
  ASSERT_EQ(-1, acm_dual_stream_->SetVAD(true, true, VADNormal));

  
  acm_dual_stream_->UnregisterSecondarySendCodec();
  
  ASSERT_EQ(-1, acm_dual_stream_->SecondarySendCodec(&my_codec));

  ASSERT_EQ(0, acm_dual_stream_->SetVAD(true, true, VADVeryAggr));
  
  EXPECT_EQ(0, acm_dual_stream_->VAD(&vad_status, &dtx_status, &vad_mode));
  EXPECT_TRUE(vad_status);
  EXPECT_TRUE(dtx_status);
  EXPECT_EQ(VADVeryAggr, vad_mode);
}

}
  
