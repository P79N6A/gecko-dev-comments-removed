









#include "delay_estimator.h"

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

void WebRtc_FreeBinaryDelayEstimator(BinaryDelayEstimator* handle) {

  if (handle == NULL) {
    return;
  }

  free(handle->mean_bit_counts);
  handle->mean_bit_counts = NULL;

  free(handle->bit_counts);
  handle->bit_counts = NULL;

  free(handle->binary_far_history);
  handle->binary_far_history = NULL;

  free(handle->binary_near_history);
  handle->binary_near_history = NULL;

  free(handle->far_bit_counts);
  handle->far_bit_counts = NULL;

  free(handle);
}

BinaryDelayEstimator* WebRtc_CreateBinaryDelayEstimator(int max_delay,
                                                        int lookahead) {
  BinaryDelayEstimator* self = NULL;
  int history_size = max_delay + lookahead;  

  if ((max_delay >= 0) && (lookahead >= 0) && (history_size > 1)) {
    
    self = malloc(sizeof(BinaryDelayEstimator));
  }

  if (self != NULL) {
    int malloc_fail = 0;

    self->mean_bit_counts = NULL;
    self->bit_counts = NULL;
    self->binary_far_history = NULL;
    self->far_bit_counts = NULL;
    self->binary_near_history = NULL;

    self->history_size = history_size;
    self->near_history_size = lookahead + 1;

    
    self->mean_bit_counts = malloc(history_size * sizeof(int32_t));
    malloc_fail |= (self->mean_bit_counts == NULL);

    self->bit_counts = malloc(history_size * sizeof(int32_t));
    malloc_fail |= (self->bit_counts == NULL);

    
    self->binary_far_history = malloc(history_size * sizeof(uint32_t));
    malloc_fail |= (self->binary_far_history == NULL);

    self->binary_near_history = malloc((lookahead + 1) * sizeof(uint32_t));
    malloc_fail |= (self->binary_near_history == NULL);

    self->far_bit_counts = malloc(history_size * sizeof(int));
    malloc_fail |= (self->far_bit_counts == NULL);

    if (malloc_fail) {
      WebRtc_FreeBinaryDelayEstimator(self);
      self = NULL;
    }
  }

  return self;
}

void WebRtc_InitBinaryDelayEstimator(BinaryDelayEstimator* handle) {
  int i = 0;
  assert(handle != NULL);

  memset(handle->bit_counts, 0, sizeof(int32_t) * handle->history_size);
  memset(handle->binary_far_history, 0,
         sizeof(uint32_t) * handle->history_size);
  memset(handle->binary_near_history, 0,
         sizeof(uint32_t) * handle->near_history_size);
  memset(handle->far_bit_counts, 0, sizeof(int) * handle->history_size);
  for (i = 0; i < handle->history_size; ++i) {
    handle->mean_bit_counts[i] = (20 << 9);  
  }
  handle->minimum_probability = (32 << 9);  
  handle->last_delay_probability = (32 << 9);  

  
  handle->last_delay = -2;
}

int WebRtc_ProcessBinarySpectrum(BinaryDelayEstimator* handle,
                                 uint32_t binary_far_spectrum,
                                 uint32_t binary_near_spectrum) {
  int i = 0;
  int candidate_delay = -1;

  int32_t value_best_candidate = 16384;  
  int32_t value_worst_candidate = 0;

  assert(handle != NULL);
  
  memmove(&(handle->binary_far_history[1]), &(handle->binary_far_history[0]),
          (handle->history_size - 1) * sizeof(uint32_t));
  handle->binary_far_history[0] = binary_far_spectrum;

  
  
  memmove(&(handle->far_bit_counts[1]), &(handle->far_bit_counts[0]),
          (handle->history_size - 1) * sizeof(int));
  handle->far_bit_counts[0] = BitCount(binary_far_spectrum);

  if (handle->near_history_size > 1) {
    
    
    memmove(&(handle->binary_near_history[1]),
            &(handle->binary_near_history[0]),
            (handle->near_history_size - 1) * sizeof(uint32_t));
    handle->binary_near_history[0] = binary_near_spectrum;
    binary_near_spectrum =
        handle->binary_near_history[handle->near_history_size - 1];
  }

  
  BitCountComparison(binary_near_spectrum,
                     handle->binary_far_history,
                     handle->history_size,
                     handle->bit_counts);

  
  for (i = 0; i < handle->history_size; i++) {
    
    
    int32_t bit_count = (handle->bit_counts[i] << 9);  

    
    
    
    if (handle->far_bit_counts[i] > 0) {
      
      int shifts = kShiftsAtZero;
      shifts -= (kShiftsLinearSlope * handle->far_bit_counts[i]) >> 4;
      WebRtc_MeanEstimatorFix(bit_count, shifts, &(handle->mean_bit_counts[i]));
    }
  }

  
  
  for (i = 0; i < handle->history_size; i++) {
    if (handle->mean_bit_counts[i] < value_best_candidate) {
      value_best_candidate = handle->mean_bit_counts[i];
      candidate_delay = i;
    }
    if (handle->mean_bit_counts[i] > value_worst_candidate) {
      value_worst_candidate = handle->mean_bit_counts[i];
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  

  
  if ((handle->minimum_probability > kProbabilityLowerLimit) &&
      (value_worst_candidate - value_best_candidate > kProbabilityMinSpread)) {
    
    
    
    
    int32_t threshold = value_best_candidate + kProbabilityOffset;
    if (threshold < kProbabilityLowerLimit) {
      threshold = kProbabilityLowerLimit;
    }
    if (handle->minimum_probability > threshold) {
      handle->minimum_probability = threshold;
    }
  }
  
  
  handle->last_delay_probability++;
  if (value_worst_candidate > value_best_candidate + kProbabilityOffset) {
    
    if (value_best_candidate < handle->minimum_probability) {
      handle->last_delay = candidate_delay;
    }
    if (value_best_candidate < handle->last_delay_probability) {
      handle->last_delay = candidate_delay;
      
      handle->last_delay_probability = value_best_candidate;
    }
  }

  return handle->last_delay;
}

int WebRtc_binary_last_delay(BinaryDelayEstimator* handle) {
  assert(handle != NULL);
  return handle->last_delay;
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
