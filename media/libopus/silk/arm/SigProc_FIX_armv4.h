



























#ifndef SILK_SIGPROC_FIX_ARMv4_H
#define SILK_SIGPROC_FIX_ARMv4_H

#undef silk_MLA
static inline opus_int32 silk_MLA_armv4(opus_int32 a, opus_int32 b,
 opus_int32 c)
{
  opus_int32 res;
  __asm__(
      "#silk_MLA\n\t"
      "mla %0, %1, %2, %3\n\t"
      : "=&r"(res)
      : "r"(b), "r"(c), "r"(a)
  );
  return res;
}
#define silk_MLA(a, b, c) (silk_MLA_armv4(a, b, c))

#endif
