









#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"

#include "webrtc/modules/audio_coding/neteq4/accelerate.h"
#include "webrtc/modules/audio_coding/neteq4/buffer_level_filter.h"
#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/delay_manager.h"
#include "webrtc/modules/audio_coding/neteq4/delay_peak_detector.h"
#include "webrtc/modules/audio_coding/neteq4/dtmf_buffer.h"
#include "webrtc/modules/audio_coding/neteq4/dtmf_tone_generator.h"
#include "webrtc/modules/audio_coding/neteq4/expand.h"
#include "webrtc/modules/audio_coding/neteq4/neteq_impl.h"
#include "webrtc/modules/audio_coding/neteq4/packet_buffer.h"
#include "webrtc/modules/audio_coding/neteq4/payload_splitter.h"
#include "webrtc/modules/audio_coding/neteq4/preemptive_expand.h"
#include "webrtc/modules/audio_coding/neteq4/timestamp_scaler.h"

namespace webrtc {



NetEq* NetEq::Create(int sample_rate_hz) {
  BufferLevelFilter* buffer_level_filter = new BufferLevelFilter;
  DecoderDatabase* decoder_database = new DecoderDatabase;
  DelayPeakDetector* delay_peak_detector = new DelayPeakDetector;
  DelayManager* delay_manager = new DelayManager(kMaxNumPacketsInBuffer,
                                                 delay_peak_detector);
  DtmfBuffer* dtmf_buffer = new DtmfBuffer(sample_rate_hz);
  DtmfToneGenerator* dtmf_tone_generator = new DtmfToneGenerator;
  PacketBuffer* packet_buffer = new PacketBuffer(kMaxNumPacketsInBuffer,
                                                 kMaxBytesInBuffer);
  PayloadSplitter* payload_splitter = new PayloadSplitter;
  TimestampScaler* timestamp_scaler = new TimestampScaler(*decoder_database);
  AccelerateFactory* accelerate_factory = new AccelerateFactory;
  ExpandFactory* expand_factory = new ExpandFactory;
  PreemptiveExpandFactory* preemptive_expand_factory =
      new PreemptiveExpandFactory;
  return new NetEqImpl(sample_rate_hz,
                       buffer_level_filter,
                       decoder_database,
                       delay_manager,
                       delay_peak_detector,
                       dtmf_buffer,
                       dtmf_tone_generator,
                       packet_buffer,
                       payload_splitter,
                       timestamp_scaler,
                       accelerate_factory,
                       expand_factory,
                       preemptive_expand_factory);
}

}  
