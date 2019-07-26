









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_COMFORT_NOISE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_COMFORT_NOISE_H_

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class DecoderDatabase;
class SyncBuffer;
struct Packet;


class ComfortNoise {
 public:
  enum ReturnCodes {
    kOK = 0,
    kUnknownPayloadType,
    kInternalError,
    kMultiChannelNotSupported
  };

  ComfortNoise(int fs_hz, DecoderDatabase* decoder_database,
               SyncBuffer* sync_buffer)
      : fs_hz_(fs_hz),
        first_call_(true),
        overlap_length_(5 * fs_hz_ / 8000),
        decoder_database_(decoder_database),
        sync_buffer_(sync_buffer),
        internal_error_code_(0) {
  }

  
  void Reset();

  
  
  int UpdateParameters(Packet* packet);

  
  
  
  
  int Generate(size_t requested_length, AudioMultiVector* output);

  
  
  int internal_error_code() { return internal_error_code_; }

 private:
  int fs_hz_;
  bool first_call_;
  size_t overlap_length_;
  DecoderDatabase* decoder_database_;
  SyncBuffer* sync_buffer_;
  int internal_error_code_;
  DISALLOW_COPY_AND_ASSIGN(ComfortNoise);
};

}  
#endif
