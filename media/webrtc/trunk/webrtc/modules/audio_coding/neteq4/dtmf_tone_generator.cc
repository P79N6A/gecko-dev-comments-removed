





























#include "webrtc/modules/audio_coding/neteq4/dtmf_tone_generator.h"

#include <assert.h>

namespace webrtc {




const int DtmfToneGenerator::kCoeff1[4][16] = {
    { 24219, 27980, 27980, 27980, 26956, 26956, 26956, 25701, 25701, 25701,
      24219, 24219, 27980, 26956, 25701, 24219 },
    { 30556, 31548, 31548, 31548, 31281, 31281, 31281, 30951, 30951, 30951,
      30556, 30556, 31548, 31281, 30951, 30556 },
    { 32210, 32462, 32462, 32462, 32394, 32394, 32394, 32311, 32311, 32311,
      32210, 32210, 32462, 32394, 32311, 32210 },
    { 32520, 32632, 32632, 32632, 32602, 32602, 32602, 32564, 32564, 32564,
      32520, 32520, 32632, 32602, 32564, 32520 } };




const int DtmfToneGenerator::kCoeff2[4][16] = {
    { 16325, 19073, 16325, 13085, 19073, 16325, 13085, 19073, 16325, 13085,
      19073, 13085, 9315, 9315, 9315, 9315},
    { 28361, 29144, 28361, 27409, 29144, 28361, 27409, 29144, 28361, 27409,
      29144, 27409, 26258, 26258, 26258, 26258},
    { 31647, 31849, 31647, 31400, 31849, 31647, 31400, 31849, 31647, 31400,
      31849, 31400, 31098, 31098, 31098, 31098},
    { 32268, 32359, 32268, 32157, 32359, 32268, 32157, 32359, 32268, 32157,
      32359, 32157, 32022, 32022, 32022, 32022} };




const int DtmfToneGenerator::kInitValue1[4][16] = {
    { 11036, 8528, 8528, 8528, 9315, 9315, 9315, 10163, 10163, 10163, 11036,
      11036, 8528, 9315, 10163, 11036},
    { 5918, 4429, 4429, 4429, 4879, 4879, 4879, 5380, 5380, 5380, 5918, 5918,
      4429, 4879, 5380, 5918},
    { 3010, 2235, 2235, 2235, 2468, 2468, 2468, 2728, 2728, 2728, 3010, 3010,
      2235, 2468, 2728, 3010},
    { 2013, 1493, 1493, 1493, 1649, 1649, 1649, 1823, 1823, 1823, 2013, 2013,
      1493, 1649, 1823, 2013 } };




const int DtmfToneGenerator::kInitValue2[4][16] = {
    { 14206, 13323, 14206, 15021, 13323, 14206, 15021, 13323, 14206, 15021,
      13323, 15021, 15708, 15708, 15708, 15708},
    { 8207, 7490, 8207, 8979, 7490, 8207, 8979, 7490, 8207, 8979, 7490, 8979,
      9801, 9801, 9801, 9801},
    { 4249, 3853, 4249, 4685, 3853, 4249, 4685, 3853, 4249, 4685, 3853, 4685,
      5164, 5164, 5164, 5164},
    { 2851, 2582, 2851, 3148, 2582, 2851, 3148, 2582, 2851, 3148, 2582, 3148,
      3476, 3476, 3476, 3476} };



const int DtmfToneGenerator::kAmplitude[37] = {
    16141, 14386, 12821, 11427, 10184, 9077, 8090, 7210, 6426, 5727, 5104, 4549,
    4054, 3614, 3221, 2870, 2558, 2280, 2032, 1811, 1614, 1439, 1282, 1143,
    1018, 908, 809, 721, 643, 573, 510, 455, 405, 361, 322, 287, 256 };


DtmfToneGenerator::DtmfToneGenerator()
    : initialized_(false),
      coeff1_(0),
      coeff2_(0),
      amplitude_(0) {
}




int DtmfToneGenerator::Init(int fs, int event, int attenuation) {
  initialized_ = false;
  int fs_index;
  if (fs == 8000) {
    fs_index = 0;
  } else if (fs == 16000) {
    fs_index = 1;
  } else if (fs == 32000) {
    fs_index = 2;
  } else if (fs == 48000) {
    fs_index = 3;
  } else {
    assert(false);
    fs_index = 1;  
  }

  if (event < 0 || event > 15) {
    return kParameterError;  
  }

  if (attenuation < 0 || attenuation > 36) {
    return kParameterError;  
  }

  
  coeff1_ = kCoeff1[fs_index][event];
  coeff2_ = kCoeff2[fs_index][event];
  
  amplitude_ = kAmplitude[attenuation];
  
  sample_history1_[0] = kInitValue1[fs_index][event];
  sample_history1_[1] = 0;
  sample_history2_[0] = kInitValue2[fs_index][event];
  sample_history2_[1] = 0;

  initialized_ = true;
  return 0;
}


void DtmfToneGenerator::Reset() {
  initialized_ = false;
}


int DtmfToneGenerator::Generate(int num_samples,
                                AudioMultiVector* output) {
  if (!initialized_) {
    return kNotInitialized;
  }

  if (num_samples < 0 || !output) {
    return kParameterError;
  }
  assert(output->Channels() == 1);  
  if (output->Channels() != 1) {
    return kStereoNotSupported;
  }

  output->AssertSize(num_samples);
  for (int i = 0; i < num_samples; ++i) {
    
    int16_t temp_val_low = ((coeff1_ * sample_history1_[1] + 8192) >> 14)
        - sample_history1_[0];
    int16_t temp_val_high = ((coeff2_ * sample_history2_[1] + 8192) >> 14)
        - sample_history2_[0];

    
    sample_history1_[0] = sample_history1_[1];
    sample_history1_[1] = temp_val_low;
    sample_history2_[0] = sample_history2_[1];
    sample_history2_[1] = temp_val_high;

    
    int32_t temp_val = kAmpMultiplier * temp_val_low + (temp_val_high << 15);
    
    temp_val = (temp_val + 16384) >> 15;
    
    (*output)[0][i] =
        static_cast<int16_t>((temp_val * amplitude_ + 8192) >> 14);
  }

  return num_samples;
}

}  
