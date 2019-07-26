

























#include "signal_processing_library.h"

#include <stdlib.h>






int16_t WebRtcSpl_MaxAbsValueW16C(const int16_t* vector, int length) {
  int i = 0, absolute = 0, maximum = 0;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  for (i = 0; i < length; i++) {
    absolute = abs((int)vector[i]);

    if (absolute > maximum) {
      maximum = absolute;
    }
  }

  
  if (maximum > WEBRTC_SPL_WORD16_MAX) {
    maximum = WEBRTC_SPL_WORD16_MAX;
  }

  return (int16_t)maximum;
}


int32_t WebRtcSpl_MaxAbsValueW32C(const int32_t* vector, int length) {
  
  

  uint32_t absolute = 0, maximum = 0;
  int i = 0;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  for (i = 0; i < length; i++) {
    absolute = abs((int)vector[i]);
    if (absolute > maximum) {
      maximum = absolute;
    }
  }

  maximum = WEBRTC_SPL_MIN(maximum, WEBRTC_SPL_WORD32_MAX);

  return (int32_t)maximum;
}


int16_t WebRtcSpl_MaxValueW16C(const int16_t* vector, int length) {
  int16_t maximum = WEBRTC_SPL_WORD16_MIN;
  int i = 0;

  if (vector == NULL || length <= 0) {
    return maximum;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] > maximum)
      maximum = vector[i];
  }
  return maximum;
}


int32_t WebRtcSpl_MaxValueW32C(const int32_t* vector, int length) {
  int32_t maximum = WEBRTC_SPL_WORD32_MIN;
  int i = 0;

  if (vector == NULL || length <= 0) {
    return maximum;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] > maximum)
      maximum = vector[i];
  }
  return maximum;
}


int16_t WebRtcSpl_MinValueW16C(const int16_t* vector, int length) {
  int16_t minimum = WEBRTC_SPL_WORD16_MAX;
  int i = 0;

  if (vector == NULL || length <= 0) {
    return minimum;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] < minimum)
      minimum = vector[i];
  }
  return minimum;
}


int32_t WebRtcSpl_MinValueW32C(const int32_t* vector, int length) {
  int32_t minimum = WEBRTC_SPL_WORD32_MAX;
  int i = 0;

  if (vector == NULL || length <= 0) {
    return minimum;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] < minimum)
      minimum = vector[i];
  }
  return minimum;
}


int WebRtcSpl_MaxAbsIndexW16(const int16_t* vector, int length) {
  

  int i = 0, absolute = 0, maximum = 0, index = 0;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  for (i = 0; i < length; i++) {
    absolute = abs((int)vector[i]);

    if (absolute > maximum) {
      maximum = absolute;
      index = i;
    }
  }

  return index;
}


int WebRtcSpl_MaxIndexW16(const int16_t* vector, int length) {
  int i = 0, index = 0;
  int16_t maximum = WEBRTC_SPL_WORD16_MIN;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] > maximum) {
      maximum = vector[i];
      index = i;
    }
  }

  return index;
}


int WebRtcSpl_MaxIndexW32(const int32_t* vector, int length) {
  int i = 0, index = 0;
  int32_t maximum = WEBRTC_SPL_WORD32_MIN;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] > maximum) {
      maximum = vector[i];
      index = i;
    }
  }

  return index;
}


int WebRtcSpl_MinIndexW16(const int16_t* vector, int length) {
  int i = 0, index = 0;
  int16_t minimum = WEBRTC_SPL_WORD16_MAX;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] < minimum) {
      minimum = vector[i];
      index = i;
    }
  }

  return index;
}


int WebRtcSpl_MinIndexW32(const int32_t* vector, int length) {
  int i = 0, index = 0;
  int32_t minimum = WEBRTC_SPL_WORD32_MAX;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  for (i = 0; i < length; i++) {
    if (vector[i] < minimum) {
      minimum = vector[i];
      index = i;
    }
  }

  return index;
}
