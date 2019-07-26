



























#ifndef SILK_SIGPROC_FIX_ARMv5E_H
#define SILK_SIGPROC_FIX_ARMv5E_H

#undef silk_SMULTT
static inline opus_int32 silk_SMULTT_armv5e(opus_int32 a, opus_int32 b)
{
  opus_int32 res;
  __asm__(
      "#silk_SMULTT\n\t"
      "smultt %0, %1, %2\n\t"
      : "=r"(res)
      : "%r"(a), "r"(b)
  );
  return res;
}
#define silk_SMULTT(a, b) (silk_SMULTT_armv5e(a, b))

#undef silk_SMLATT
static inline opus_int32 silk_SMLATT_armv5e(opus_int32 a, opus_int32 b,
 opus_int32 c)
{
  opus_int32 res;
  __asm__(
      "#silk_SMLATT\n\t"
      "smlatt %0, %1, %2, %3\n\t"
      : "=r"(res)
      : "%r"(b), "r"(c), "r"(a)
  );
  return res;
}
#define silk_SMLATT(a, b, c) (silk_SMLATT_armv5e(a, b, c))

#endif
