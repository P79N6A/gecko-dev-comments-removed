









#include "webrtc/voice_engine/test/auto_test/fixtures/before_initialization_fixture.h"

#include <stdlib.h>

class VoeBaseMiscTest : public BeforeInitializationFixture {
};

using namespace testing;

TEST_F(VoeBaseMiscTest, GetVersionPrintsSomeUsefulInformation) {
  char char_buffer[1024];
  memset(char_buffer, 0, sizeof(char_buffer));
  EXPECT_EQ(0, voe_base_->GetVersion(char_buffer));
  EXPECT_THAT(char_buffer, ContainsRegex("VoiceEngine"));
}
