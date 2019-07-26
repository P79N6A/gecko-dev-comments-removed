









#include "webrtc/modules/audio_processing/utility/delay_estimator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>



static const int kShiftsAtZero = 13;  
static const int kShiftsLinearSlope = 3;

static const int32_t kProbabilityOffset = 1024;  
static const int32_t kProbabilityLowerLimit = 8704;  
static const int32_t kProbabilityMinSpread = 2816;  


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

    
    self->mean_bit_counts = malloc(farend->history_size * sizeof(int32_t));
    malloc_fail |= (self->mean_bit_counts == NULL);

    self->bit_counts = malloc(farend->history_size * sizeof(int32_t));
    malloc_fail |= (self->bit_counts == NULL);

    
    self->binary_near_history = malloc((lookahead + 1) * sizeof(uint32_t));
    malloc_fail |= (self->binary_near_history == NULL);

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
  for (i = 0; i < self->farend->history_size; ++i) {
    self->mean_bit_counts[i] = (20 << 9);  
  }
  self->minimum_probability = (32 << 9);  
  self->last_delay_probability = (32 << 9);  

  
  self->last_delay = -2;
}

int WebRtc_ProcessBinarySpectrum(BinaryDelayEstimator* self,
                                 uint32_t binary_near_spectrum) {
  int i = 0;
  int candidate_delay = -1;

  int32_t value_best_candidate = 32 << 9;  
  int32_t value_worst_candidate = 0;

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

  
  
  
  
  
  
  
  
  
  
  
  

  
  if ((self->minimum_probability > kProbabilityLowerLimit) &&
      (value_worst_candidate - value_best_candidate > kProbabilityMinSpread)) {
    
    
    
    
    int32_t threshold = value_best_candidate + kProbabilityOffset;
    if (threshold < kProbabilityLowerLimit) {
      threshold = kProbabilityLowerLimit;
    }
    if (self->minimum_probability > threshold) {
      self->minimum_probability = threshold;
    }
  }
  
  
  self->last_delay_probability++;
  if (value_worst_candidate > value_best_candidate + kProbabilityOffset) {
    
    if (value_best_candidate < self->minimum_probability) {
      self->last_delay = candidate_delay;
    }
    if (value_best_candidate < self->last_delay_probability) {
      self->last_delay = candidate_delay;
      
      self->last_delay_probability = value_best_candidate;
    }
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
