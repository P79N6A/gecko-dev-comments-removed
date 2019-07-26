









#include "signal_processing_library.h"


void WebRtcSpl_CrossCorrelationC(int32_t* cross_correlation,
                                 const int16_t* seq1,
                                 const int16_t* seq2,
                                 int16_t dim_seq,
                                 int16_t dim_cross_correlation,
                                 int16_t right_shifts,
                                 int16_t step_seq2) {
  int i = 0, j = 0;

  for (i = 0; i < dim_cross_correlation; i++) {
    *cross_correlation = 0;
    
    for (j = 0; j < dim_seq; j++) {
      *cross_correlation += (seq1[j] * seq2[step_seq2 * i + j]) >> right_shifts;
    }
    cross_correlation++;
  }
}
