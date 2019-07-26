









#include "webrtc/modules/audio_coding/neteq4/comfort_noise.h"

#include <assert.h>

#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/dsp_helper.h"
#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"
#include "webrtc/modules/audio_coding/neteq4/sync_buffer.h"

namespace webrtc {

void ComfortNoise::Reset() {
  first_call_ = true;
  internal_error_code_ = 0;
}

int ComfortNoise::UpdateParameters(Packet* packet) {
  assert(packet);  
  
  AudioDecoder* cng_decoder = decoder_database_->GetDecoder(
      packet->header.payloadType);
  if (!cng_decoder) {
    delete [] packet->payload;
    delete packet;
    return kUnknownPayloadType;
  }
  decoder_database_->SetActiveCngDecoder(packet->header.payloadType);
  CNG_dec_inst* cng_inst = static_cast<CNG_dec_inst*>(cng_decoder->state());
  int16_t ret = WebRtcCng_UpdateSid(cng_inst,
                                    packet->payload,
                                    packet->payload_length);
  delete [] packet->payload;
  delete packet;
  if (ret < 0) {
    internal_error_code_ = WebRtcCng_GetErrorCodeDec(cng_inst);
    return kInternalError;
  }
  return kOK;
}

int ComfortNoise::Generate(size_t requested_length,
                           AudioMultiVector* output) {
  
  assert(fs_hz_ == 8000 || fs_hz_ == 16000 || fs_hz_ ==  32000 ||
         fs_hz_ == 48000);
  
  if (output->Channels() != 1) {
    return kMultiChannelNotSupported;
  }

  size_t number_of_samples = requested_length;
  int16_t new_period = 0;
  if (first_call_) {
    
    number_of_samples = requested_length + overlap_length_;
    new_period = 1;
  }
  output->AssertSize(number_of_samples);
  
  AudioDecoder* cng_decoder = decoder_database_->GetActiveCngDecoder();
  if (!cng_decoder) {
    return kUnknownPayloadType;
  }
  CNG_dec_inst* cng_inst = static_cast<CNG_dec_inst*>(cng_decoder->state());
  
  
  if (WebRtcCng_Generate(cng_inst, &(*output)[0][0],
                         static_cast<int16_t>(number_of_samples),
                         new_period) < 0) {
    
    output->Zeros(requested_length);
    internal_error_code_ = WebRtcCng_GetErrorCodeDec(cng_inst);
    return kInternalError;
  }

  if (first_call_) {
    
    int16_t muting_window;  
    int16_t muting_window_increment;  
    int16_t unmuting_window;  
    int16_t unmuting_window_increment;  
    if (fs_hz_ == 8000) {
      muting_window = DspHelper::kMuteFactorStart8kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement8kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart8kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement8kHz;
    } else if (fs_hz_ == 16000) {
      muting_window = DspHelper::kMuteFactorStart16kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement16kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart16kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement16kHz;
    } else if (fs_hz_ == 32000) {
      muting_window = DspHelper::kMuteFactorStart32kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement32kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart32kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement32kHz;
    } else {  
      muting_window = DspHelper::kMuteFactorStart48kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement48kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart48kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement48kHz;
    }

    
    size_t start_ix = sync_buffer_->Size() - overlap_length_;
    for (size_t i = 0; i < overlap_length_; i++) {
      
      
      
      (*sync_buffer_)[0][start_ix + i] =
          (((*sync_buffer_)[0][start_ix + i] * muting_window) +
              ((*output)[0][i] * unmuting_window) + 16384) >> 15;
      muting_window += muting_window_increment;
      unmuting_window += unmuting_window_increment;
    }
    
    
    output->PopFront(overlap_length_);
  }
  first_call_ = false;
  return kOK;
}

}  
