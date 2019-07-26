









#include "common_types.h"
#include "before_initialization_fixture.h"

using namespace webrtc;

class HardwareBeforeInitializingTest : public BeforeInitializationFixture {
};

TEST_F(HardwareBeforeInitializingTest,
       SetAudioDeviceLayerAcceptsPlatformDefaultBeforeInitializing) {
  AudioLayers wanted_layer = kAudioPlatformDefault;
  AudioLayers given_layer;
  EXPECT_EQ(0, voe_hardware_->SetAudioDeviceLayer(wanted_layer));
  EXPECT_EQ(0, voe_hardware_->GetAudioDeviceLayer(given_layer));
  EXPECT_EQ(wanted_layer, given_layer) <<
      "These should be the same before initializing.";
}
