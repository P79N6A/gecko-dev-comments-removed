









#include "before_initialization_fixture.h"

class VoeBaseMiscTest : public BeforeInitializationFixture {
};

using namespace testing;

TEST_F(VoeBaseMiscTest, MaxNumChannelsIs32) {
  EXPECT_EQ(32, voe_base_->MaxNumOfChannels());
}

TEST_F(VoeBaseMiscTest, GetVersionPrintsSomeUsefulInformation) {
  char char_buffer[1024];
  EXPECT_EQ(0, voe_base_->GetVersion(char_buffer));
  EXPECT_THAT(char_buffer, ContainsRegex("VoiceEngine"));
}
