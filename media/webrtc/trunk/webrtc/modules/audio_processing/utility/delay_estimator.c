









#include "webrtc/modules/audio_processing/utility/delay_estimator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>



static const int kShiftsAtZero = 13;  
static const int kShiftsLinearSlope = 3;

static const int32_t kProbabilityOffset = 1024;  
static const int32_t kProbabilityLowerLimit = 8704;  
static const int32_t kProbabilityMinSpread = 2816;  


static const float kHistogramMax = 3000.f;
static const float kLastHistogramMax = 250.f;
static const float kMinHistogramThreshold = 1.5f;
static const int kMinRequiredHits = 10;
static const int kMaxHitsWhenPossiblyNonCausal = 10;
static const int kMaxHitsWhenPossiblyCausal = 1000;
static const float kQ14Scaling = 1.f / (1 << 14);  
static const float kFractionSlope = 0.05f;
static const float kMinFractionWhenPossiblyCausal = 0.5f;
static const float kMinFractionWhenPossiblyNonCausal = 0.25f;


static int BitCount(uint32_t u32) {
  uint32_t tmp = u32 - ((u32 >> 1) & 033333333333) -
      ((u32 >> 2) & 011111111111);
  tmp = ((tmp + (tmp >> 3)) & 030707070707);
  tmp = (tmp + (tmp >> 6));
  tmp = (tmp + (tmp >> 12) + (tmp >> 24)) & 077;

  return ((int) tmp);
}














static void BitCountComparison(uint32_t binary_vector,
                               const uint32_t* binary_matrix,
                               int matrix_size,
                               int32_t* bit_counts) {
  int n = 0;

  
  for (; n < matrix_size; n++) {
    bit_counts[n] = (int32_t) BitCount(binary_vector ^ binary_matrix[n]);
  }
}


















static void UpdateRobustValidationStatistics(BinaryDelayEstimator* self,
                                             int candidate_delay,
                                             int32_t valley_depth_q14,
                                             int32_t valley_level_q14) {
  const float valley_depth = valley_depth_q14 * kQ14Scaling;
  float decrease_in_last_set = valley_depth;
  const int max_hits_for_slow_change = (candidate_delay < self->last_delay) ?
      kMaxHitsWhenPossiblyNonCausal : kMaxHitsWhenPossiblyCausal;
  int i = 0;

  
  if (candidate_delay != self->last_candidate_delay) {
    self->candidate_hits = 0;
    self->last_candidate_delay = candidate_delay;
  }
  self->candidate_hits++;

  
  
  
  
  
  self->histogram[candidate_delay] += valley_depth;
  if (self->histogram[candidate_delay] > kHistogramMax) {
    self->histogram[candidate_delay] = kHistogramMax;
  }
  
  
  
  
  
  
  
  
  
  if (self->candidate_hits < max_hits_for_slow_change) {
    decrease_in_last_set = (self->mean_bit_counts[self->compare_delay] -
        valley_level_q14) * kQ14Scaling;
  }
  
  
  
  for (i = 0; i < self->farend->history_size; ++i) {
    int is_in_last_set = (i >= self->last_delay - 2) &&
        (i <= self->last_delay + 1) && (i != candidate_delay);
    int is_in_candidate_set = (i >= candidate_delay - 2) &&
        (i <= candidate_delay + 1);
    self->histogram[i] -= decrease_in_last_set * is_in_last_set +
        valley_depth * (!is_in_last_set && !is_in_candidate_set);
    
    if (self->histogram[i] < 0) {
      self->histogram[i] = 0;
    }
  }
}























static int HistogramBasedValidation(const BinaryDelayEstimator* self,
                                    int candidate_delay) {
  float fraction = 1.f;
  float histogram_threshold = self->histogram[self->compare_delay];
  const int delay_difference = candidate_delay - self->last_delay;
  int is_histogram_valid = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  if (delay_difference > self->allowed_offset) {
    fraction = 1.f - kFractionSlope * (delay_difference - self->allowed_offset);
    fraction = (fraction > kMinFractionWhenPossiblyCausal ? fraction :
        kMinFractionWhenPossiblyCausal);
  } else if (delay_difference < 0) {
    fraction = kMinFractionWhenPossiblyNonCausal -
        kFractionSlope * delay_difference;
    fraction = (fraction > 1.f ? 1.f : fraction);
  }
  histogram_threshold *= fraction;
  histogram_threshold = (histogram_threshold > kMinHistogramThreshold ?
      histogram_threshold : kMinHistogramThreshold);

  is_histogram_valid =
      (self->histogram[candidate_delay] >= histogram_threshold) &&
      (self->candidate_hits > kMinRequiredHits);

  return is_histogram_valid;
}


















static int RobustValidation(const BinaryDelayEstimator* self,
                            int candidate_delay,
                            int is_instantaneous_valid,
                            int is_histogram_valid) {
  int is_robust = 0;

  
  
  
  
  
  
  is_robust = (self->last_delay < 0) &&
      (is_instantaneous_valid || is_histogram_valid);
  
  
  is_robust |= is_instantaneous_valid && is_histogram_valid;
  
  
  
  is_robust |= is_histogram_valid &&
      (self->histogram[candidate_delay] > self->last_delay_histogram);

  return is_robust;
}

void WebRtc_FreeBinaryDelayEstimatorFarend(BinaryDelayEstimatorFarend* self) {

  if (self == NULL) {
    return;
  }

  free(self->binary_far_history);
  self->binary_far_history = NULL;

  free(self->far_bit_counts);
  self->far_bit_counts = NULL;

  free(self);
}

BinaryDelayEstimatorFarend* WebRtc_CreateBinaryDelayEstimatorFarend(
    int history_size) {
  BinaryDelayEstimatorFarend* self = NULL;

  if (history_size > 1) {
    
    self = malloc(sizeof(BinaryDelayEstimatorFarend));
  }
  if (self != NULL) {
    int malloc_fail = 0;

    self->history_size = history_size;

    
    self->binary_far_history = malloc(history_size * sizeof(uint32_t));
    malloc_fail |= (self->binary_far_history == NULL);

    self->far_bit_counts = malloc(history_size * sizeof(int));
    malloc_fail |= (self->far_bit_counts == NULL);

    if (malloc_fail) {
      WebRtc_FreeBinaryDelayEstimatorFarend(self);
      self = NULL;
    }
  }

  return self;
}

void WebRtc_InitBinaryDelayEstimatorFarend(BinaryDelayEstimatorFarend* self) {
  assert(self != NULL);
  memset(self->binary_far_history, 0, sizeof(uint32_t) * self->history_size);
  memset(self->far_bit_counts, 0, sizeof(int) * self->history_size);
}

void WebRtc_AddBinaryFarSpectrum(BinaryDelayEstimatorFarend* handle,
                                 uint32_t binary_far_spectrum) {
  assert(handle != NULL);
  
  memmove(&(handle->binary_far_history[1]), &(handle->binary_far_history[0]),
          (handle->history_size - 1) * sizeof(uint32_t));
  handle->binary_far_history[0] = binary_far_spectrum;

  
  
  memmove(&(handle->far_bit_counts[1]), &(handle->far_bit_counts[0]),
          (handle->history_size - 1) * sizeof(int));
  handle->far_bit_counts[0] = BitCount(binary_far_spectrum);
}

void WebRtc_FreeBinaryDelayEstimator(BinaryDelayEstimator* self) {

  if (self == NULL) {
    return;
  }

  free(self->mean_bit_counts);
  self->mean_bit_counts = NULL;

  free(self->bit_counts);
  self->bit_counts = NULL;

  free(self->binary_near_history);
  self->binary_near_history = NULL;

  free(self->histogram);
  self->histogram = NULL;

  
  
  self->farend = NULL;

  free(self);
}

BinaryDelayEstimator* WebRtc_CreateBinaryDelayEstimator(
    BinaryDelayEstimatorFarend* farend, int lookahead) {
  BinaryDelayEstimator* self = NULL;

  if ((farend != NULL) && (lookahead >= 0)) {
    
    self = malloc(sizeof(BinaryDelayEstimator));
  }

  if (self != NULL) {
    int malloc_fail = 0;

    self->farend = farend;
    self->near_history_size = lookahead + 1;
    self->robust_validation_enabled = 0;  
    self->allowed_offset = 0;

    
    
    
    self->mean_bit_counts =
        malloc((farend->history_size + 1) * sizeof(int32_t));
    malloc_fail |= (self->mean_bit_counts == NULL);

    self->bit_counts = malloc(farend->history_size * sizeof(int32_t));
    malloc_fail |= (self->bit_counts == NULL);

    
    self->binary_near_history = malloc((lookahead + 1) * sizeof(uint32_t));
    malloc_fail |= (self->binary_near_history == NULL);

    self->histogram = malloc((farend->history_size + 1) * sizeof(float));
    malloc_fail |= (self->histogram == NULL);

    if (malloc_fail) {
      WebRtc_FreeBinaryDelayEstimator(self);
      self = NULL;
    }
  }

  return self;
}

void WebRtc_InitBinaryDelayEstimator(BinaryDelayEstimator* self) {
  int i = 0;
  assert(self != NULL);

  memset(self->bit_counts, 0, sizeof(int32_t) * self->farend->history_size);
  memset(self->binary_near_history, 0,
         sizeof(uint32_t) * self->near_history_size);
  for (i = 0; i <= self->farend->history_size; ++i) {
    self->mean_bit_counts[i] = (20 << 9);  
    self->histogram[i] = 0.f;
  }
  self->minimum_probability = (32 << 9);  
  self->last_delay_probability = (32 << 9);  

  
  self->last_delay = -2;

  self->last_candidate_delay = -2;
  self->compare_delay = self->farend->history_size;
  self->candidate_hits = 0;
  self->last_delay_histogram = 0.f;
}

int WebRtc_ProcessBinarySpectrum(BinaryDelayEstimator* self,
                                 uint32_t binary_near_spectrum) {
  int i = 0;
  int candidate_delay = -1;
  int valid_candidate = 0;

  int32_t value_best_candidate = 32 << 9;  
  int32_t value_worst_candidate = 0;
  int32_t valley_depth = 0;

  assert(self != NULL);
  if (self->near_history_size > 1) {
    
    
    memmove(&(self->binary_near_history[1]), &(self->binary_near_history[0]),
            (self->near_history_size - 1) * sizeof(uint32_t));
    self->binary_near_history[0] = binary_near_spectrum;
    binary_near_spectrum =
        self->binary_near_history[self->near_history_size - 1];
  }

  
  BitCountComparison(binary_near_spectrum, self->farend->binary_far_history,
                     self->farend->history_size, self->bit_counts);

  
  for (i = 0; i < self->farend->history_size; i++) {
    
    
    int32_t bit_count = (self->bit_counts[i] << 9);  

    
    
    
    if (self->farend->far_bit_counts[i] > 0) {
      
      int shifts = kShiftsAtZero;
      shifts -= (kShiftsLinearSlope * self->farend->far_bit_counts[i]) >> 4;
      WebRtc_MeanEstimatorFix(bit_count, shifts, &(self->mean_bit_counts[i]));
    }
  }

  
  
  for (i = 0; i < self->farend->history_size; i++) {
    if (self->mean_bit_counts[i] < value_best_candidate) {
      value_best_candidate = self->mean_bit_counts[i];
      candidate_delay = i;
    }
    if (self->mean_bit_counts[i] > value_worst_candidate) {
      value_worst_candidate = self->mean_bit_counts[i];
    }
  }
  valley_depth = value_worst_candidate - value_best_candidate;

  
  
  
  
  
  
  
  
  
  
  
  

  
  if ((self->minimum_probability > kProbabilityLowerLimit) &&
      (valley_depth > kProbabilityMinSpread)) {
    
    
    
    
    int32_t threshold = value_best_candidate + kProbabilityOffset;
    if (threshold < kProbabilityLowerLimit) {
      threshold = kProbabilityLowerLimit;
    }
    if (self->minimum_probability > threshold) {
      self->minimum_probability = threshold;
    }
  }
  
  
  self->last_delay_probability++;
  
  
  
  
  
  
  
  
  valid_candidate = ((valley_depth > kProbabilityOffset) &&
      ((value_best_candidate < self->minimum_probability) ||
          (value_best_candidate < self->last_delay_probability)));

  if (self->robust_validation_enabled) {
    int is_histogram_valid = 0;
    UpdateRobustValidationStatistics(self, candidate_delay, valley_depth,
                                     value_best_candidate);
    is_histogram_valid = HistogramBasedValidation(self, candidate_delay);
    valid_candidate = RobustValidation(self, candidate_delay, valid_candidate,
                                       is_histogram_valid);

  }
  if (valid_candidate) {
    if (candidate_delay != self->last_delay) {
      self->last_delay_histogram =
          (self->histogram[candidate_delay] > kLastHistogramMax ?
              kLastHistogramMax : self->histogram[candidate_delay]);
      
      
      if (self->histogram[candidate_delay] <
          self->histogram[self->compare_delay]) {
        self->histogram[self->compare_delay] = self->histogram[candidate_delay];
      }
    }
    self->last_delay = candidate_delay;
    if (value_best_candidate < self->last_delay_probability) {
      self->last_delay_probability = value_best_candidate;
    }
    self->compare_delay = self->last_delay;
  }

  return self->last_delay;
}

int WebRtc_binary_last_delay(BinaryDelayEstimator* self) {
  assert(self != NULL);
  return self->last_delay;
}

int WebRtc_binary_last_delay_quality(BinaryDelayEstimator* self) {
  int delay_quality = 0;
  assert(self != NULL);
  
  
  
  
  

  
  delay_quality = (32 << 9) - self->last_delay_probability;
  if (delay_quality < 0) {
    delay_quality = 0;
  }
  return delay_quality;
}

void WebRtc_MeanEstimatorFix(int32_t new_value,
                             int factor,
                             int32_t* mean_value) {
  int32_t diff = new_value - *mean_value;

  
  if (diff < 0) {
    diff = -((-diff) >> factor);
  } else {
    diff = (diff >> factor);
  }
  *mean_value += diff;
}
