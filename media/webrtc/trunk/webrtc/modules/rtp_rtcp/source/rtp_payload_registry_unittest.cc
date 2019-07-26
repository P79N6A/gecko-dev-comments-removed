









#include "webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/rtp_rtcp/source/mock/mock_rtp_payload_strategy.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

using ::testing::Eq;
using ::testing::Return;
using ::testing::_;

static const char* kTypicalPayloadName = "name";
static const uint8_t kTypicalChannels = 1;
static const int kTypicalFrequency = 44000;
static const int kTypicalRate = 32 * 1024;

class RtpPayloadRegistryTest : public ::testing::Test {
 public:
  void SetUp() {
    
    mock_payload_strategy_ = new testing::NiceMock<MockRTPPayloadStrategy>();
    rtp_payload_registry_.reset(
        new RTPPayloadRegistry(123, mock_payload_strategy_));
  }

 protected:
  ModuleRTPUtility::Payload* ExpectReturnOfTypicalAudioPayload(
      uint8_t payload_type, uint32_t rate) {
    bool audio = true;
    ModuleRTPUtility::Payload returned_payload = { "name", audio, {
        
        { kTypicalFrequency, kTypicalChannels, rate }
    }};

    
    
    ModuleRTPUtility::Payload* returned_payload_on_heap =
        new ModuleRTPUtility::Payload(returned_payload);
    EXPECT_CALL(*mock_payload_strategy_,
        CreatePayloadType(kTypicalPayloadName, payload_type,
            kTypicalFrequency,
            kTypicalChannels,
            rate)).WillOnce(Return(returned_payload_on_heap));
    return returned_payload_on_heap;
  }

  scoped_ptr<RTPPayloadRegistry> rtp_payload_registry_;
  testing::NiceMock<MockRTPPayloadStrategy>* mock_payload_strategy_;
};

TEST_F(RtpPayloadRegistryTest, RegistersAndRemembersPayloadsUntilDeregistered) {
  uint8_t payload_type = 97;
  ModuleRTPUtility::Payload* returned_payload_on_heap =
      ExpectReturnOfTypicalAudioPayload(payload_type, kTypicalRate);

  bool new_payload_created = false;
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type, kTypicalFrequency, kTypicalChannels,
      kTypicalRate, &new_payload_created));

  EXPECT_TRUE(new_payload_created) << "A new payload WAS created.";

  ModuleRTPUtility::Payload* retrieved_payload = NULL;
  EXPECT_TRUE(rtp_payload_registry_->PayloadTypeToPayload(payload_type,
                                                          retrieved_payload));

  
  
  EXPECT_EQ(returned_payload_on_heap, retrieved_payload);

  
  EXPECT_EQ(0, rtp_payload_registry_->DeRegisterReceivePayload(payload_type));
  EXPECT_FALSE(rtp_payload_registry_->PayloadTypeToPayload(
      payload_type, retrieved_payload));
}

TEST_F(RtpPayloadRegistryTest, DoesNotCreateNewPayloadTypeIfRed) {
  EXPECT_CALL(*mock_payload_strategy_,
      CreatePayloadType(_, _, _, _, _)).Times(0);

  bool new_payload_created = false;
  uint8_t red_type_of_the_day = 104;
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      "red", red_type_of_the_day, kTypicalFrequency, kTypicalChannels,
      kTypicalRate, &new_payload_created));
  ASSERT_FALSE(new_payload_created);

  ASSERT_EQ(red_type_of_the_day, rtp_payload_registry_->red_payload_type());

  ModuleRTPUtility::Payload* retrieved_payload = NULL;
  EXPECT_TRUE(rtp_payload_registry_->PayloadTypeToPayload(red_type_of_the_day,
                                                          retrieved_payload));
  EXPECT_FALSE(retrieved_payload->audio);
  EXPECT_STRCASEEQ("red", retrieved_payload->name);
}

TEST_F(RtpPayloadRegistryTest,
       DoesNotAcceptSamePayloadTypeTwiceExceptIfPayloadIsCompatible) {
  uint8_t payload_type = 97;

  bool ignored = false;
  ModuleRTPUtility::Payload* first_payload_on_heap =
      ExpectReturnOfTypicalAudioPayload(payload_type, kTypicalRate);
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type, kTypicalFrequency, kTypicalChannels,
      kTypicalRate, &ignored));

  EXPECT_EQ(-1, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type, kTypicalFrequency, kTypicalChannels,
      kTypicalRate, &ignored)) << "Adding same codec twice = bad.";

  ModuleRTPUtility::Payload* second_payload_on_heap =
      ExpectReturnOfTypicalAudioPayload(payload_type - 1, kTypicalRate);
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type - 1, kTypicalFrequency,
      kTypicalChannels, kTypicalRate, &ignored)) <<
          "With a different payload type is fine though.";

  
  ModuleRTPUtility::Payload* retrieved_payload = NULL;
  EXPECT_TRUE(rtp_payload_registry_->PayloadTypeToPayload(payload_type,
                                                          retrieved_payload));
  EXPECT_EQ(first_payload_on_heap, retrieved_payload);
  EXPECT_TRUE(rtp_payload_registry_->PayloadTypeToPayload(payload_type - 1,
                                                          retrieved_payload));
  EXPECT_EQ(second_payload_on_heap, retrieved_payload);

  
  
  
  ON_CALL(*mock_payload_strategy_, PayloadIsCompatible(_, _, _, _))
      .WillByDefault(Return(true));
  EXPECT_CALL(*mock_payload_strategy_,
              UpdatePayloadRate(first_payload_on_heap, kTypicalRate));
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type, kTypicalFrequency, kTypicalChannels,
      kTypicalRate, &ignored));
}

TEST_F(RtpPayloadRegistryTest,
       RemovesCompatibleCodecsOnRegistryIfCodecsMustBeUnique) {
  ON_CALL(*mock_payload_strategy_, PayloadIsCompatible(_, _, _, _))
      .WillByDefault(Return(true));
  ON_CALL(*mock_payload_strategy_, CodecsMustBeUnique())
      .WillByDefault(Return(true));

  uint8_t payload_type = 97;

  bool ignored = false;
  ExpectReturnOfTypicalAudioPayload(payload_type, kTypicalRate);
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type, kTypicalFrequency, kTypicalChannels,
      kTypicalRate, &ignored));
  ExpectReturnOfTypicalAudioPayload(payload_type - 1, kTypicalRate);
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type - 1, kTypicalFrequency,
      kTypicalChannels, kTypicalRate, &ignored));

  ModuleRTPUtility::Payload* retrieved_payload = NULL;
  EXPECT_FALSE(rtp_payload_registry_->PayloadTypeToPayload(
      payload_type, retrieved_payload)) << "The first payload should be "
          "deregistered because the only thing that differs is payload type.";
  EXPECT_TRUE(rtp_payload_registry_->PayloadTypeToPayload(
      payload_type - 1, retrieved_payload)) <<
          "The second payload should still be registered though.";

  
  ON_CALL(*mock_payload_strategy_, PayloadIsCompatible(_, _, _, _))
      .WillByDefault(Return(false));
  ExpectReturnOfTypicalAudioPayload(payload_type + 1, kTypicalRate);
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, payload_type + 1, kTypicalFrequency,
      kTypicalChannels, kTypicalRate, &ignored));

  EXPECT_TRUE(rtp_payload_registry_->PayloadTypeToPayload(
      payload_type - 1, retrieved_payload)) <<
          "Not compatible; both payloads should be kept.";
  EXPECT_TRUE(rtp_payload_registry_->PayloadTypeToPayload(
      payload_type + 1, retrieved_payload)) <<
          "Not compatible; both payloads should be kept.";
}

TEST_F(RtpPayloadRegistryTest,
       LastReceivedCodecTypesAreResetWhenRegisteringNewPayloadTypes) {
  rtp_payload_registry_->set_last_received_payload_type(17);
  EXPECT_EQ(17, rtp_payload_registry_->last_received_payload_type());

  bool media_type_unchanged = rtp_payload_registry_->ReportMediaPayloadType(18);
  EXPECT_FALSE(media_type_unchanged);
  media_type_unchanged = rtp_payload_registry_->ReportMediaPayloadType(18);
  EXPECT_TRUE(media_type_unchanged);

  bool ignored;
  ExpectReturnOfTypicalAudioPayload(34, kTypicalRate);
  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload(
      kTypicalPayloadName, 34, kTypicalFrequency, kTypicalChannels,
      kTypicalRate, &ignored));

  EXPECT_EQ(-1, rtp_payload_registry_->last_received_payload_type());
  media_type_unchanged = rtp_payload_registry_->ReportMediaPayloadType(18);
  EXPECT_FALSE(media_type_unchanged);
}

class ParameterizedRtpPayloadRegistryTest :
    public RtpPayloadRegistryTest,
    public ::testing::WithParamInterface<int> {
};

TEST_P(ParameterizedRtpPayloadRegistryTest,
       FailsToRegisterKnownPayloadsWeAreNotInterestedIn) {
  int payload_type = GetParam();

  bool ignored;
  EXPECT_EQ(-1, rtp_payload_registry_->RegisterReceivePayload(
      "whatever", static_cast<uint8_t>(payload_type), 19, 1, 17,
      &ignored));
}

INSTANTIATE_TEST_CASE_P(TestKnownBadPayloadTypes,
                        ParameterizedRtpPayloadRegistryTest,
                        testing::Values(64, 72, 73, 74, 75, 76, 77, 78, 79));

class RtpPayloadRegistryGenericTest :
    public RtpPayloadRegistryTest,
    public ::testing::WithParamInterface<int> {
};

TEST_P(RtpPayloadRegistryGenericTest, RegisterGenericReceivePayloadType) {
  int payload_type = GetParam();

  bool ignored;

  EXPECT_EQ(0, rtp_payload_registry_->RegisterReceivePayload("generic-codec",
    static_cast<int8_t>(payload_type),
    19, 1, 17, &ignored)); 
}

INSTANTIATE_TEST_CASE_P(TestDynamicRange, RtpPayloadRegistryGenericTest,
                        testing::Range(96, 127+1));

}  
