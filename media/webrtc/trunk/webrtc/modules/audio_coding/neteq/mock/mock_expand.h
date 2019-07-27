









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_MOCK_MOCK_EXPAND_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_MOCK_MOCK_EXPAND_H_

#include "webrtc/modules/audio_coding/neteq/expand.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace webrtc {

class MockExpand : public Expand {
 public:
  MockExpand(BackgroundNoise* background_noise,
             SyncBuffer* sync_buffer,
             RandomVector* random_vector,
             int fs,
             size_t num_channels)
      : Expand(background_noise, sync_buffer, random_vector, fs, num_channels) {
  }
  virtual ~MockExpand() { Die(); }
  MOCK_METHOD0(Die, void());
  MOCK_METHOD0(Reset,
      void());
  MOCK_METHOD1(Process,
      int(AudioMultiVector* output));
  MOCK_METHOD0(SetParametersForNormalAfterExpand,
      void());
  MOCK_METHOD0(SetParametersForMergeAfterExpand,
      void());
  MOCK_CONST_METHOD0(overlap_length,
      size_t());
};

}  

namespace webrtc {

class MockExpandFactory : public ExpandFactory {
 public:
  MOCK_CONST_METHOD5(Create,
                     Expand*(BackgroundNoise* background_noise,
                             SyncBuffer* sync_buffer,
                             RandomVector* random_vector,
                             int fs,
                             size_t num_channels));
};

}  
#endif  
