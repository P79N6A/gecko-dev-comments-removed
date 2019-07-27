











#include "webrtc/modules/audio_coding/neteq/comfort_noise.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq/mock/mock_decoder_database.h"
#include "webrtc/modules/audio_coding/neteq/sync_buffer.h"

namespace webrtc {

TEST(ComfortNoise, CreateAndDestroy) {
  int fs = 8000;
  MockDecoderDatabase db;
  SyncBuffer sync_buffer(1, 1000);
  ComfortNoise cn(fs, &db, &sync_buffer);
  EXPECT_CALL(db, Die());  
}



}  
