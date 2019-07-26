

















#include "signal_processing_library.h"


int16_t WebRtcSpl_MaxAbsValueW16_mips(const int16_t* vector, int length) {
  int32_t totMax = 0;
  int32_t tmp32_0, tmp32_1, tmp32_2, tmp32_3;
  int i, loop_size;

  if (vector == NULL || length <= 0) {
    return -1;
  }
#if defined(MIPS_DSP_R1)
  const int32_t* tmpvec32 = (int32_t*)vector;
  loop_size = length >> 4;

  for (i = 0; i < loop_size; i++) {
    __asm__ volatile (
      "lw         %[tmp32_0],     0(%[tmpvec32])              \n\t"
      "lw         %[tmp32_1],     4(%[tmpvec32])              \n\t"
      "lw         %[tmp32_2],     8(%[tmpvec32])              \n\t"
      "lw         %[tmp32_3],     12(%[tmpvec32])             \n\t"

      "absq_s.ph  %[tmp32_0],     %[tmp32_0]                  \n\t"
      "absq_s.ph  %[tmp32_1],     %[tmp32_1]                  \n\t"
      "cmp.lt.ph  %[totMax],      %[tmp32_0]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_0],     %[totMax]   \n\t"

      "lw         %[tmp32_0],     16(%[tmpvec32])             \n\t"
      "absq_s.ph  %[tmp32_2],     %[tmp32_2]                  \n\t"
      "cmp.lt.ph  %[totMax],      %[tmp32_1]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_1],     %[totMax]   \n\t"

      "lw         %[tmp32_1],     20(%[tmpvec32])             \n\t"
      "absq_s.ph  %[tmp32_3],     %[tmp32_3]                  \n\t"
      "cmp.lt.ph  %[totMax],      %[tmp32_2]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_2],     %[totMax]   \n\t"

      "lw         %[tmp32_2],     24(%[tmpvec32])             \n\t"
      "cmp.lt.ph  %[totMax],      %[tmp32_3]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_3],     %[totMax]   \n\t"

      "lw         %[tmp32_3],     28(%[tmpvec32])             \n\t"
      "absq_s.ph  %[tmp32_0],     %[tmp32_0]                  \n\t"
      "absq_s.ph  %[tmp32_1],     %[tmp32_1]                  \n\t"
      "cmp.lt.ph  %[totMax],      %[tmp32_0]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_0],     %[totMax]   \n\t"

      "absq_s.ph  %[tmp32_2],     %[tmp32_2]                  \n\t"
      "cmp.lt.ph  %[totMax],      %[tmp32_1]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_1],     %[totMax]   \n\t"
      "absq_s.ph  %[tmp32_3],     %[tmp32_3]                  \n\t"
      "cmp.lt.ph  %[totMax],      %[tmp32_2]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_2],     %[totMax]   \n\t"

      "cmp.lt.ph  %[totMax],      %[tmp32_3]                  \n\t"
      "pick.ph    %[totMax],      %[tmp32_3],     %[totMax]   \n\t"

      "addiu      %[tmpvec32],    %[tmpvec32],    32          \n\t"
      : [tmp32_0] "=&r" (tmp32_0), [tmp32_1] "=&r" (tmp32_1),
        [tmp32_2] "=&r" (tmp32_2), [tmp32_3] "=&r" (tmp32_3),
        [totMax] "+r" (totMax), [tmpvec32] "+r" (tmpvec32)
      :
      : "memory"
    );
  }
  __asm__ volatile (
    "rotr       %[tmp32_0],     %[totMax],      16          \n\t"
    "cmp.lt.ph  %[totMax],      %[tmp32_0]                  \n\t"
    "pick.ph    %[totMax],      %[tmp32_0],     %[totMax]   \n\t"
    "packrl.ph  %[totMax],      $0,             %[totMax]   \n\t"
    : [tmp32_0] "=&r" (tmp32_0), [totMax] "+r" (totMax)
    :
  );
  loop_size = length & 0xf;
  for (i = 0; i < loop_size; i++) {
    __asm__ volatile (
      "lh         %[tmp32_0],     0(%[tmpvec32])              \n\t"
      "addiu      %[tmpvec32],    %[tmpvec32],     2          \n\t"
      "absq_s.w   %[tmp32_0],     %[tmp32_0]                  \n\t"
      "slt        %[tmp32_1],     %[totMax],       %[tmp32_0] \n\t"
      "movn       %[totMax],      %[tmp32_0],      %[tmp32_1] \n\t"
      : [tmp32_0] "=&r" (tmp32_0), [tmp32_1] "=&r" (tmp32_1),
        [tmpvec32] "+r" (tmpvec32), [totMax] "+r" (totMax)
      :
      : "memory"
    );
  }
#else  
  int32_t v16MaxMax = WEBRTC_SPL_WORD16_MAX;
  int32_t r, r1, r2, r3;
  const int16_t* tmpvector = vector;
  loop_size = length >> 4;
  for (i = 0; i < loop_size; i++) {
    __asm__ volatile (
      "lh     %[tmp32_0],     0(%[tmpvector])                 \n\t"
      "lh     %[tmp32_1],     2(%[tmpvector])                 \n\t"
      "lh     %[tmp32_2],     4(%[tmpvector])                 \n\t"
      "lh     %[tmp32_3],     6(%[tmpvector])                 \n\t"

      "abs    %[tmp32_0],     %[tmp32_0]                      \n\t"
      "abs    %[tmp32_1],     %[tmp32_1]                      \n\t"
      "abs    %[tmp32_2],     %[tmp32_2]                      \n\t"
      "abs    %[tmp32_3],     %[tmp32_3]                      \n\t"

      "slt    %[r],           %[totMax],      %[tmp32_0]      \n\t"
      "movn   %[totMax],      %[tmp32_0],     %[r]            \n\t"
      "slt    %[r1],          %[totMax],      %[tmp32_1]      \n\t"
      "movn   %[totMax],      %[tmp32_1],     %[r1]           \n\t"
      "slt    %[r2],          %[totMax],      %[tmp32_2]      \n\t"
      "movn   %[totMax],      %[tmp32_2],     %[r2]           \n\t"
      "slt    %[r3],          %[totMax],      %[tmp32_3]      \n\t"
      "movn   %[totMax],      %[tmp32_3],     %[r3]           \n\t"

      "lh     %[tmp32_0],     8(%[tmpvector])                 \n\t"
      "lh     %[tmp32_1],     10(%[tmpvector])                \n\t"
      "lh     %[tmp32_2],     12(%[tmpvector])                \n\t"
      "lh     %[tmp32_3],     14(%[tmpvector])                \n\t"

      "abs    %[tmp32_0],     %[tmp32_0]                      \n\t"
      "abs    %[tmp32_1],     %[tmp32_1]                      \n\t"
      "abs    %[tmp32_2],     %[tmp32_2]                      \n\t"
      "abs    %[tmp32_3],     %[tmp32_3]                      \n\t"

      "slt    %[r],           %[totMax],      %[tmp32_0]      \n\t"
      "movn   %[totMax],      %[tmp32_0],     %[r]            \n\t"
      "slt    %[r1],          %[totMax],      %[tmp32_1]      \n\t"
      "movn   %[totMax],      %[tmp32_1],     %[r1]           \n\t"
      "slt    %[r2],          %[totMax],      %[tmp32_2]      \n\t"
      "movn   %[totMax],      %[tmp32_2],     %[r2]           \n\t"
      "slt    %[r3],          %[totMax],      %[tmp32_3]      \n\t"
      "movn   %[totMax],      %[tmp32_3],     %[r3]           \n\t"

      "lh     %[tmp32_0],     16(%[tmpvector])                \n\t"
      "lh     %[tmp32_1],     18(%[tmpvector])                \n\t"
      "lh     %[tmp32_2],     20(%[tmpvector])                \n\t"
      "lh     %[tmp32_3],     22(%[tmpvector])                \n\t"

      "abs    %[tmp32_0],     %[tmp32_0]                      \n\t"
      "abs    %[tmp32_1],     %[tmp32_1]                      \n\t"
      "abs    %[tmp32_2],     %[tmp32_2]                      \n\t"
      "abs    %[tmp32_3],     %[tmp32_3]                      \n\t"

      "slt    %[r],           %[totMax],      %[tmp32_0]      \n\t"
      "movn   %[totMax],      %[tmp32_0],     %[r]            \n\t"
      "slt    %[r1],          %[totMax],      %[tmp32_1]      \n\t"
      "movn   %[totMax],      %[tmp32_1],     %[r1]           \n\t"
      "slt    %[r2],          %[totMax],      %[tmp32_2]      \n\t"
      "movn   %[totMax],      %[tmp32_2],     %[r2]           \n\t"
      "slt    %[r3],          %[totMax],      %[tmp32_3]      \n\t"
      "movn   %[totMax],      %[tmp32_3],     %[r3]           \n\t"

      "lh     %[tmp32_0],     24(%[tmpvector])                \n\t"
      "lh     %[tmp32_1],     26(%[tmpvector])                \n\t"
      "lh     %[tmp32_2],     28(%[tmpvector])                \n\t"
      "lh     %[tmp32_3],     30(%[tmpvector])                \n\t"

      "abs    %[tmp32_0],     %[tmp32_0]                      \n\t"
      "abs    %[tmp32_1],     %[tmp32_1]                      \n\t"
      "abs    %[tmp32_2],     %[tmp32_2]                      \n\t"
      "abs    %[tmp32_3],     %[tmp32_3]                      \n\t"

      "slt    %[r],           %[totMax],      %[tmp32_0]      \n\t"
      "movn   %[totMax],      %[tmp32_0],     %[r]            \n\t"
      "slt    %[r1],          %[totMax],      %[tmp32_1]      \n\t"
      "movn   %[totMax],      %[tmp32_1],     %[r1]           \n\t"
      "slt    %[r2],          %[totMax],      %[tmp32_2]      \n\t"
      "movn   %[totMax],      %[tmp32_2],     %[r2]           \n\t"
      "slt    %[r3],          %[totMax],      %[tmp32_3]      \n\t"
      "movn   %[totMax],      %[tmp32_3],     %[r3]           \n\t"

      "addiu  %[tmpvector],   %[tmpvector],   32              \n\t"
      : [tmp32_0] "=&r" (tmp32_0), [tmp32_1] "=&r" (tmp32_1),
        [tmp32_2] "=&r" (tmp32_2), [tmp32_3] "=&r" (tmp32_3),
        [totMax] "+r" (totMax), [r] "=&r" (r), [tmpvector] "+r" (tmpvector),
        [r1] "=&r" (r1), [r2] "=&r" (r2), [r3] "=&r" (r3)
      :
      : "memory"
    );
  }
  loop_size = length & 0xf;
  for (i = 0; i < loop_size; i++) {
    __asm__ volatile (
      "lh         %[tmp32_0],     0(%[tmpvector])             \n\t"
      "addiu      %[tmpvector],   %[tmpvector],    2          \n\t"
      "abs        %[tmp32_0],     %[tmp32_0]                  \n\t"
      "slt        %[tmp32_1],     %[totMax],       %[tmp32_0] \n\t"
      "movn       %[totMax],      %[tmp32_0],      %[tmp32_1] \n\t"
      : [tmp32_0] "=&r" (tmp32_0), [tmp32_1] "=&r" (tmp32_1),
        [tmpvector] "+r" (tmpvector), [totMax] "+r" (totMax)
      :
      : "memory"
    );
  }

  __asm__ volatile (
    "slt    %[r],       %[v16MaxMax],   %[totMax]   \n\t"
    "movn   %[totMax],  %[v16MaxMax],   %[r]        \n\t"
    : [totMax] "+r" (totMax), [r] "=&r" (r)
    : [v16MaxMax] "r" (v16MaxMax)
  );
#endif  
  return (int16_t)totMax;
}

#if defined(MIPS_DSP_R1_LE)

int32_t WebRtcSpl_MaxAbsValueW32_mips(const int32_t* vector, int length) {
  
  

  uint32_t absolute = 0, maximum = 0;
  int tmp1 = 0, max_value = 0x7fffffff;

  if (vector == NULL || length <= 0) {
    return -1;
  }

  __asm__ volatile (
    ".set push                                                        \n\t"
    ".set noreorder                                                   \n\t"

   "1:                                                                \n\t"
    "lw         %[absolute],      0(%[vector])                        \n\t"
    "absq_s.w   %[absolute],      %[absolute]                         \n\t"
    "addiu      %[length],        %[length],          -1              \n\t"
    "slt        %[tmp1],          %[maximum],         %[absolute]     \n\t"
    "movn       %[maximum],       %[absolute],        %[tmp1]         \n\t"
    "bgtz       %[length],        1b                                  \n\t"
    " addiu     %[vector],        %[vector],          4               \n\t"
    "slt        %[tmp1],          %[max_value],       %[maximum]      \n\t"
    "movn       %[maximum],       %[max_value],       %[tmp1]         \n\t"

    ".set pop                                                         \n\t"

    : [tmp1] "=&r" (tmp1), [maximum] "+r" (maximum), [absolute] "+r" (absolute)
    : [vector] "r" (vector), [length] "r" (length), [max_value] "r" (max_value)
    : "memory"
  );

  return (int32_t)maximum;
}
#endif  


int16_t WebRtcSpl_MaxValueW16_mips(const int16_t* vector, int length) {
  int16_t maximum = WEBRTC_SPL_WORD16_MIN;
  int tmp1;
  int16_t value;

  if (vector == NULL || length <= 0) {
    return maximum;
  }

  __asm__ volatile (
    ".set push                                                        \n\t"
    ".set noreorder                                                   \n\t"

   "1:                                                                \n\t"
    "lh         %[value],         0(%[vector])                        \n\t"
    "addiu      %[length],        %[length],          -1              \n\t"
    "slt        %[tmp1],          %[maximum],         %[value]        \n\t"
    "movn       %[maximum],       %[value],           %[tmp1]         \n\t"
    "bgtz       %[length],        1b                                  \n\t"
    " addiu     %[vector],        %[vector],          2               \n\t"
    ".set pop                                                         \n\t"

    : [tmp1] "=&r" (tmp1), [maximum] "+r" (maximum), [value] "=&r" (value)
    : [vector] "r" (vector), [length] "r" (length)
    : "memory"
  );

  return maximum;
}


int32_t WebRtcSpl_MaxValueW32_mips(const int32_t* vector, int length) {
  int32_t maximum = WEBRTC_SPL_WORD32_MIN;
  int tmp1, value;

  if (vector == NULL || length <= 0) {
    return maximum;
  }

  __asm__ volatile (
    ".set push                                                        \n\t"
    ".set noreorder                                                   \n\t"

   "1:                                                                \n\t"
    "lw         %[value],         0(%[vector])                        \n\t"
    "addiu      %[length],        %[length],          -1              \n\t"
    "slt        %[tmp1],          %[maximum],         %[value]        \n\t"
    "movn       %[maximum],       %[value],           %[tmp1]         \n\t"
    "bgtz       %[length],        1b                                  \n\t"
    " addiu     %[vector],        %[vector],          4               \n\t"

    ".set pop                                                         \n\t"

    : [tmp1] "=&r" (tmp1), [maximum] "+r" (maximum), [value] "=&r" (value)
    : [vector] "r" (vector), [length] "r" (length)
    : "memory"
  );

  return maximum;
}


int16_t WebRtcSpl_MinValueW16_mips(const int16_t* vector, int length) {
  int16_t minimum = WEBRTC_SPL_WORD16_MAX;
  int tmp1;
  int16_t value;

  if (vector == NULL || length <= 0) {
    return minimum;
  }

  __asm__ volatile (
    ".set push                                                        \n\t"
    ".set noreorder                                                   \n\t"

   "1:                                                                \n\t"
    "lh         %[value],         0(%[vector])                        \n\t"
    "addiu      %[length],        %[length],          -1              \n\t"
    "slt        %[tmp1],          %[value],           %[minimum]      \n\t"
    "movn       %[minimum],       %[value],           %[tmp1]         \n\t"
    "bgtz       %[length],        1b                                  \n\t"
    " addiu     %[vector],        %[vector],          2               \n\t"

    ".set pop                                                         \n\t"

    : [tmp1] "=&r" (tmp1), [minimum] "+r" (minimum), [value] "=&r" (value)
    : [vector] "r" (vector), [length] "r" (length)
    : "memory"
  );

  return minimum;
}


int32_t WebRtcSpl_MinValueW32_mips(const int32_t* vector, int length) {
  int32_t minimum = WEBRTC_SPL_WORD32_MAX;
  int tmp1, value;

  if (vector == NULL || length <= 0) {
    return minimum;
  }

  __asm__ volatile (
    ".set push                                                        \n\t"
    ".set noreorder                                                   \n\t"

   "1:                                                                \n\t"
    "lw         %[value],         0(%[vector])                        \n\t"
    "addiu      %[length],        %[length],          -1              \n\t"
    "slt        %[tmp1],          %[value],           %[minimum]      \n\t"
    "movn       %[minimum],       %[value],           %[tmp1]         \n\t"
    "bgtz       %[length],        1b                                  \n\t"
    " addiu     %[vector],        %[vector],          4               \n\t"

    ".set pop                                                         \n\t"

    : [tmp1] "=&r" (tmp1), [minimum] "+r" (minimum), [value] "=&r" (value)
    : [vector] "r" (vector), [length] "r" (length)
    : "memory"
  );

  return minimum;
}

