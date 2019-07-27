









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_SEND_TEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_SEND_TEST_H_

#include <vector>

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/neteq/tools/packet_source.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

namespace test {
class InputAudioFile;
class Packet;

class AcmSendTestOldApi : public AudioPacketizationCallback,
                          public PacketSource {
 public:
  AcmSendTestOldApi(InputAudioFile* audio_source,
                    int source_rate_hz,
                    int test_duration_ms);
  virtual ~AcmSendTestOldApi() {}

  
  bool RegisterCodec(const char* payload_name,
                     int sampling_freq_hz,
                     int channels,
                     int payload_type,
                     int frame_size_samples);

  
  
  
  Packet* NextPacket();

  
  virtual int32_t SendData(
      FrameType frame_type,
      uint8_t payload_type,
      uint32_t timestamp,
      const uint8_t* payload_data,
      uint16_t payload_len_bytes,
      const RTPFragmentationHeader* fragmentation) OVERRIDE;

 private:
  static const int kBlockSizeMs = 10;

  
  
  
  Packet* CreatePacket();

  SimulatedClock clock_;
  scoped_ptr<AudioCodingModule> acm_;
  InputAudioFile* audio_source_;
  int source_rate_hz_;
  const int input_block_size_samples_;
  AudioFrame input_frame_;
  CodecInst codec_;
  bool codec_registered_;
  int test_duration_ms_;
  
  FrameType frame_type_;
  int payload_type_;
  uint32_t timestamp_;
  uint16_t sequence_number_;
  std::vector<uint8_t> last_payload_vec_;

  DISALLOW_COPY_AND_ASSIGN(AcmSendTestOldApi);
};

}  
}  
#endif  
