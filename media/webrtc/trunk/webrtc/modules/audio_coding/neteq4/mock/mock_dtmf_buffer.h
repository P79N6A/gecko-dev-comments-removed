









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DTMF_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DTMF_BUFFER_H_

#include "webrtc/modules/audio_coding/neteq4/dtmf_buffer.h"

#include "gmock/gmock.h"

namespace webrtc {

class MockDtmfBuffer : public DtmfBuffer {
 public:
  MockDtmfBuffer(int fs) : DtmfBuffer(fs) {}
  virtual ~MockDtmfBuffer() { Die(); }
  MOCK_METHOD0(Die, void());
  MOCK_METHOD0(Flush,
      void());
  MOCK_METHOD1(InsertEvent,
      int(const DtmfEvent& event));
  MOCK_METHOD2(GetEvent,
      bool(uint32_t current_timestamp, DtmfEvent* event));
  MOCK_CONST_METHOD0(Length,
      size_t());
  MOCK_CONST_METHOD0(Empty,
      bool());
};

}  
#endif  
