









#include "vad_filterbank.h"

#include <assert.h>

#include "signal_processing_library.h"
#include "typedefs.h"


static const int16_t kLogConst = 24660;  
static const int16_t kLogEnergyIntPart = 14336;  


static const int16_t kHpZeroCoefs[3] = { 6631, -13262, 6631 };
static const int16_t kHpPoleCoefs[3] = { 16384, -7756, 5620 };



static const int16_t kAllPassCoefsQ15[2] = { 20972, 5571 };


static const int16_t kOffsetVector[6] = { 368, 368, 272, 176, 176, 176 };









static void HighPassFilter(const int16_t* data_in, int data_length,
                           int16_t* filter_state, int16_t* data_out) {
  int i;
  const int16_t* in_ptr = data_in;
  int16_t* out_ptr = data_out;
  int32_t tmp32 = 0;


  
  
  
  
  
  
  

  for (i = 0; i < data_length; i++) {
    
    tmp32 = WEBRTC_SPL_MUL_16_16(kHpZeroCoefs[0], *in_ptr);
    tmp32 += WEBRTC_SPL_MUL_16_16(kHpZeroCoefs[1], filter_state[0]);
    tmp32 += WEBRTC_SPL_MUL_16_16(kHpZeroCoefs[2], filter_state[1]);
    filter_state[1] = filter_state[0];
    filter_state[0] = *in_ptr++;

    
    tmp32 -= WEBRTC_SPL_MUL_16_16(kHpPoleCoefs[1], filter_state[2]);
    tmp32 -= WEBRTC_SPL_MUL_16_16(kHpPoleCoefs[2], filter_state[3]);
    filter_state[3] = filter_state[2];
    filter_state[2] = (int16_t) (tmp32 >> 14);
    *out_ptr++ = filter_state[2];
  }
}










static void AllPassFilter(const int16_t* data_in, int data_length,
                          int16_t filter_coefficient, int16_t* filter_state,
                          int16_t* data_out) {
  
  
  
  
  

  int i;
  int16_t tmp16 = 0;
  int32_t tmp32 = 0;
  int32_t state32 = ((int32_t) (*filter_state) << 16);  

  for (i = 0; i < data_length; i++) {
    tmp32 = state32 + WEBRTC_SPL_MUL_16_16(filter_coefficient, *data_in);
    tmp16 = (int16_t) (tmp32 >> 16);  
    *data_out++ = tmp16;
    state32 = (((int32_t) (*data_in)) << 14); 
    state32 -= WEBRTC_SPL_MUL_16_16(filter_coefficient, tmp16);  
    state32 <<= 1;  
    data_in += 2;
  }

  *filter_state = (int16_t) (state32 >> 16);  
}












static void SplitFilter(const int16_t* data_in, int data_length,
                        int16_t* upper_state, int16_t* lower_state,
                        int16_t* hp_data_out, int16_t* lp_data_out) {
  int i;
  int half_length = data_length >> 1;  
  int16_t tmp_out;

  
  AllPassFilter(&data_in[0], half_length, kAllPassCoefsQ15[0], upper_state,
                hp_data_out);

  
  AllPassFilter(&data_in[1], half_length, kAllPassCoefsQ15[1], lower_state,
                lp_data_out);

  
  for (i = 0; i < half_length; i++) {
    tmp_out = *hp_data_out;
    *hp_data_out++ -= *lp_data_out;
    *lp_data_out++ += tmp_out;
  }
}












static void LogOfEnergy(const int16_t* data_in, int data_length,
                        int16_t offset, int16_t* total_energy,
                        int16_t* log_energy) {
  
  int tot_rshifts = 0;
  
  
  uint32_t energy = 0;

  assert(data_in != NULL);
  assert(data_length > 0);

  energy = (uint32_t) WebRtcSpl_Energy((int16_t*) data_in, data_length,
                                       &tot_rshifts);

  if (energy != 0) {
    
    
    int normalizing_rshifts = 17 - WebRtcSpl_NormU32(energy);
    
    
    
    int16_t log2_energy = kLogEnergyIntPart;

    tot_rshifts += normalizing_rshifts;
    
    
    
    
    if (normalizing_rshifts < 0) {
      energy <<= -normalizing_rshifts;
    } else {
      energy >>= normalizing_rshifts;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    log2_energy += (int16_t) ((energy & 0x00003FFF) >> 4);

    
    
    *log_energy = (int16_t) (WEBRTC_SPL_MUL_16_16_RSFT(
        kLogConst, log2_energy, 19) +
        WEBRTC_SPL_MUL_16_16_RSFT(tot_rshifts, kLogConst, 9));

    if (*log_energy < 0) {
      *log_energy = 0;
    }
  } else {
    *log_energy = offset;
    return;
  }

  *log_energy += offset;

  
  
  
  if (*total_energy <= kMinEnergy) {
    if (tot_rshifts >= 0) {
      
      
      *total_energy += kMinEnergy + 1;
    } else {
      
      
      
      
      *total_energy += (int16_t) (energy >> -tot_rshifts);  
    }
  }
}

int16_t WebRtcVad_CalculateFeatures(VadInstT* self, const int16_t* data_in,
                                    int data_length, int16_t* features) {
  int16_t total_energy = 0;
  
  
  
  
  int16_t hp_120[120], lp_120[120];
  int16_t hp_60[60], lp_60[60];
  const int half_data_length = data_length >> 1;
  int length = half_data_length;  
                                  

  
  int frequency_band = 0;
  const int16_t* in_ptr = data_in;  
  int16_t* hp_out_ptr = hp_120;  
  int16_t* lp_out_ptr = lp_120;  

  assert(data_length >= 0);
  assert(data_length <= 240);
  assert(4 < kNumChannels - 1);  

  
  SplitFilter(in_ptr, data_length, &self->upper_state[frequency_band],
              &self->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  
  frequency_band = 1;
  in_ptr = hp_120;  
  hp_out_ptr = hp_60;  
  lp_out_ptr = lp_60;  
  SplitFilter(in_ptr, length, &self->upper_state[frequency_band],
              &self->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  
  length >>= 1;  

  LogOfEnergy(hp_60, length, kOffsetVector[5], &total_energy, &features[5]);

  
  LogOfEnergy(lp_60, length, kOffsetVector[4], &total_energy, &features[4]);

  
  frequency_band = 2;
  in_ptr = lp_120;  
  hp_out_ptr = hp_60;  
  lp_out_ptr = lp_60;  
  length = half_data_length;  
  SplitFilter(in_ptr, length, &self->upper_state[frequency_band],
              &self->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  
  length >>= 1;  
  LogOfEnergy(hp_60, length, kOffsetVector[3], &total_energy, &features[3]);

  
  frequency_band = 3;
  in_ptr = lp_60;  
  hp_out_ptr = hp_120;  
  lp_out_ptr = lp_120;  
  SplitFilter(in_ptr, length, &self->upper_state[frequency_band],
              &self->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  
  length >>= 1;  
  LogOfEnergy(hp_120, length, kOffsetVector[2], &total_energy, &features[2]);

  
  frequency_band = 4;
  in_ptr = lp_120;  
  hp_out_ptr = hp_60;  
  lp_out_ptr = lp_60;  
  SplitFilter(in_ptr, length, &self->upper_state[frequency_band],
              &self->lower_state[frequency_band], hp_out_ptr, lp_out_ptr);

  
  length >>= 1;  
  LogOfEnergy(hp_60, length, kOffsetVector[1], &total_energy, &features[1]);

  
  HighPassFilter(lp_60, length, self->hp_filter_state, hp_120);

  
  LogOfEnergy(hp_120, length, kOffsetVector[0], &total_energy, &features[0]);

  return total_energy;
}
