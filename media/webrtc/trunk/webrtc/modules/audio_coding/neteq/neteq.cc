









#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"

#include "webrtc/modules/audio_coding/neteq/accelerate.h"
#include "webrtc/modules/audio_coding/neteq/buffer_level_filter.h"
#include "webrtc/modules/audio_coding/neteq/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq/delay_manager.h"
#include "webrtc/modules/audio_coding/neteq/delay_peak_detector.h"
#include "webrtc/modules/audio_coding/neteq/dtmf_buffer.h"
#include "webrtc/modules/audio_coding/neteq/dtmf_tone_generator.h"
#include "webrtc/modules/audio_coding/neteq/expand.h"
#include "webrtc/modules/audio_coding/neteq/neteq_impl.h"
#include "webrtc/modules/audio_coding/neteq/packet_buffer.h"
#include "webrtc/modules/audio_coding/neteq/payload_splitter.h"
#include "webrtc/modules/audio_coding/neteq/preemptive_expand.h"
#include "webrtc/modules/audio_coding/neteq/timestamp_scaler.h"

namespace webrtc {



NetEq* NetEq::Create(const NetEq::Config& config) {
  BufferLevelFilter* buffer_level_filter = new BufferLevelFilter;
  DecoderDatabase* decoder_database = new DecoderDatabase;
  DelayPeakDetector* delay_peak_detector = new DelayPeakDetector;
  DelayManager* delay_manager =
      new DelayManager(config.max_packets_in_buffer, delay_peak_detector);
  delay_manager->SetMaximumDelay(config.max_delay_ms);
  DtmfBuffer* dtmf_buffer = new DtmfBuffer(config.sample_rate_hz);
  DtmfToneGenerator* dtmf_tone_generator = new DtmfToneGenerator;
  PacketBuffer* packet_buffer = new PacketBuffer(config.max_packets_in_buffer);
  PayloadSplitter* payload_splitter = new PayloadSplitter;
  TimestampScaler* timestamp_scaler = new TimestampScaler(*decoder_database);
  AccelerateFactory* accelerate_factory = new AccelerateFactory;
  ExpandFactory* expand_factory = new ExpandFactory;
  PreemptiveExpandFactory* preemptive_expand_factory =
      new PreemptiveExpandFactory;
  return new NetEqImpl(config,
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
