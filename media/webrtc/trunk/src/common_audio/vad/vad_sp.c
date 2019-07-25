









#include "vad_sp.h"

#include <assert.h>

#include "signal_processing_library.h"
#include "typedefs.h"
#include "vad_core.h"



static const int16_t kAllPassCoefsQ13[2] = { 5243, 1392 };  
static const int16_t kSmoothingDown = 6553;  
static const int16_t kSmoothingUp = 32439;  



void WebRtcVad_Downsampling(int16_t* signal_in,
                            int16_t* signal_out,
                            int32_t* filter_state,
                            int in_length) {
  int16_t tmp16_1 = 0, tmp16_2 = 0;
  int32_t tmp32_1 = filter_state[0];
  int32_t tmp32_2 = filter_state[1];
  int n = 0;
  int half_length = (in_length >> 1);  

  
  for (n = 0; n < half_length; n++) {
    
    tmp16_1 = (int16_t) ((tmp32_1 >> 1) +
        WEBRTC_SPL_MUL_16_16_RSFT(kAllPassCoefsQ13[0], *signal_in, 14));
    *signal_out = tmp16_1;
    tmp32_1 = (int32_t) (*signal_in++) -
        WEBRTC_SPL_MUL_16_16_RSFT(kAllPassCoefsQ13[0], tmp16_1, 12);

    
    tmp16_2 = (int16_t) ((tmp32_2 >> 1) +
        WEBRTC_SPL_MUL_16_16_RSFT(kAllPassCoefsQ13[1], *signal_in, 14));
    *signal_out++ += tmp16_2;
    tmp32_2 = (int32_t) (*signal_in++) -
        WEBRTC_SPL_MUL_16_16_RSFT(kAllPassCoefsQ13[1], tmp16_2, 12);
  }
  
  filter_state[0] = tmp32_1;
  filter_state[1] = tmp32_2;
}




int16_t WebRtcVad_FindMinimum(VadInstT* self,
                              int16_t feature_value,
                              int channel) {
  int i = 0, j = 0;
  int position = -1;
  
  int offset = (channel << 4);
  int16_t current_median = 1600;
  int16_t alpha = 0;
  int32_t tmp32 = 0;
  
  
  int16_t* age_ptr = &self->index_vector[offset];
  int16_t* value_ptr = &self->low_value_vector[offset];
  int16_t *p1, *p2, *p3;

  assert(channel < kNumChannels);

  
  
  for (i = 0; i < 16; i++) {
    p3 = age_ptr + i;
    if (*p3 != 100) {
      *p3 += 1;
    } else {
      p1 = value_ptr + i + 1;
      p2 = p3 + 1;
      for (j = i; j < 16; j++) {
        *(value_ptr + j) = *p1++;
        *(age_ptr + j) = *p2++;
      }
      *(age_ptr + 15) = 101;
      *(value_ptr + 15) = 10000;
    }
  }

  
  
  
  if (feature_value < *(value_ptr + 7)) {
    if (feature_value < *(value_ptr + 3)) {
      if (feature_value < *(value_ptr + 1)) {
        if (feature_value < *value_ptr) {
          position = 0;
        } else {
          position = 1;
        }
      } else if (feature_value < *(value_ptr + 2)) {
        position = 2;
      } else {
        position = 3;
      }
    } else if (feature_value < *(value_ptr + 5)) {
      if (feature_value < *(value_ptr + 4)) {
        position = 4;
      } else {
        position = 5;
      }
    } else if (feature_value < *(value_ptr + 6)) {
      position = 6;
    } else {
      position = 7;
    }
  } else if (feature_value < *(value_ptr + 15)) {
    if (feature_value < *(value_ptr + 11)) {
      if (feature_value < *(value_ptr + 9)) {
        if (feature_value < *(value_ptr + 8)) {
          position = 8;
        } else {
          position = 9;
        }
      } else if (feature_value < *(value_ptr + 10)) {
        position = 10;
      } else {
        position = 11;
      }
    } else if (feature_value < *(value_ptr + 13)) {
      if (feature_value < *(value_ptr + 12)) {
        position = 12;
      } else {
        position = 13;
      }
    } else if (feature_value < *(value_ptr + 14)) {
      position = 14;
    } else {
      position = 15;
    }
  }

  
  
  if (position > -1) {
    for (i = 15; i > position; i--) {
      j = i - 1;
      *(value_ptr + i) = *(value_ptr + j);
      *(age_ptr + i) = *(age_ptr + j);
    }
    *(value_ptr + position) = feature_value;
    *(age_ptr + position) = 1;
  }

  
  if (self->frame_counter > 2) {
    current_median = *(value_ptr + 2);
  } else if (self->frame_counter > 0) {
    current_median = *value_ptr;
  }

  
  if (self->frame_counter > 0) {
    if (current_median < self->mean_value[channel]) {
      alpha = kSmoothingDown;  
    } else {
      alpha = kSmoothingUp;  
    }
  }
  tmp32 = WEBRTC_SPL_MUL_16_16(alpha + 1, self->mean_value[channel]);
  tmp32 += WEBRTC_SPL_MUL_16_16(WEBRTC_SPL_WORD16_MAX - alpha, current_median);
  tmp32 += 16384;
  self->mean_value[channel] = (int16_t) (tmp32 >> 15);

  return self->mean_value[channel];
}
