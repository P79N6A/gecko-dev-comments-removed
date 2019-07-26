



























#ifndef SILK_MACROS_ARMv5E_H
#define SILK_MACROS_ARMv5E_H


#undef silk_SMULWB
static inline opus_int32 silk_SMULWB_armv5e(opus_int32 a, opus_int16 b)
{
  int res;
  __asm__(
      "#silk_SMULWB\n\t"
      "smulwb %0, %1, %2\n\t"
      : "=r"(res)
      : "r"(a), "r"(b)
  );
  return res;
}
#define silk_SMULWB(a, b) (silk_SMULWB_armv5e(a, b))


#undef silk_SMLAWB
static inline opus_int32 silk_SMLAWB_armv5e(opus_int32 a, opus_int32 b,
 opus_int16 c)
{
  int res;
  __asm__(
      "#silk_SMLAWB\n\t"
      "smlawb %0, %1, %2, %3\n\t"
      : "=r"(res)
      : "r"(b), "r"(c), "r"(a)
  );
  return res;
}
#define silk_SMLAWB(a, b, c) (silk_SMLAWB_armv5e(a, b, c))


#undef silk_SMULWT
static inline opus_int32 silk_SMULWT_armv5e(opus_int32 a, opus_int32 b)
{
  int res;
  __asm__(
      "#silk_SMULWT\n\t"
      "smulwt %0, %1, %2\n\t"
      : "=r"(res)
      : "r"(a), "r"(b)
  );
  return res;
}
#define silk_SMULWT(a, b) (silk_SMULWT_armv5e(a, b))


#undef silk_SMLAWT
static inline opus_int32 silk_SMLAWT_armv5e(opus_int32 a, opus_int32 b,
 opus_int32 c)
{
  int res;
  __asm__(
      "#silk_SMLAWT\n\t"
      "smlawt %0, %1, %2, %3\n\t"
      : "=r"(res)
      : "r"(b), "r"(c), "r"(a)
  );
  return res;
}
#define silk_SMLAWT(a, b, c) (silk_SMLAWT_armv5e(a, b, c))


#undef silk_SMULBB
static inline opus_int32 silk_SMULBB_armv5e(opus_int32 a, opus_int32 b)
{
  int res;
  __asm__(
      "#silk_SMULBB\n\t"
      "smulbb %0, %1, %2\n\t"
      : "=r"(res)
      : "%r"(a), "r"(b)
  );
  return res;
}
#define silk_SMULBB(a, b) (silk_SMULBB_armv5e(a, b))


#undef silk_SMLABB
static inline opus_int32 silk_SMLABB_armv5e(opus_int32 a, opus_int32 b,
 opus_int32 c)
{
  int res;
  __asm__(
      "#silk_SMLABB\n\t"
      "smlabb %0, %1, %2, %3\n\t"
      : "=r"(res)
      : "%r"(b), "r"(c), "r"(a)
  );
  return res;
}
#define silk_SMLABB(a, b, c) (silk_SMLABB_armv5e(a, b, c))


#undef silk_SMULBT
static inline opus_int32 silk_SMULBT_armv5e(opus_int32 a, opus_int32 b)
{
  int res;
  __asm__(
      "#silk_SMULBT\n\t"
      "smulbt %0, %1, %2\n\t"
      : "=r"(res)
      : "r"(a), "r"(b)
  );
  return res;
}
#define silk_SMULBT(a, b) (silk_SMULBT_armv5e(a, b))


#undef silk_SMLABT
static inline opus_int32 silk_SMLABT_armv5e(opus_int32 a, opus_int32 b,
 opus_int32 c)
{
  int res;
  __asm__(
      "#silk_SMLABT\n\t"
      "smlabt %0, %1, %2, %3\n\t"
      : "=r"(res)
      : "r"(b), "r"(c), "r"(a)
  );
  return res;
}
#define silk_SMLABT(a, b, c) (silk_SMLABT_armv5e(a, b, c))


#undef silk_ADD_SAT32
static inline opus_int32 silk_ADD_SAT32_armv5e(opus_int32 a, opus_int32 b)
{
  int res;
  __asm__(
      "#silk_ADD_SAT32\n\t"
      "qadd %0, %1, %2\n\t"
      : "=r"(res)
      : "%r"(a), "r"(b)
  );
  return res;
}
#define silk_ADD_SAT32(a, b) (silk_ADD_SAT32_armv5e(a, b))

#undef silk_SUB_SAT32
static inline opus_int32 silk_SUB_SAT32_armv5e(opus_int32 a, opus_int32 b)
{
  int res;
  __asm__(
      "#silk_SUB_SAT32\n\t"
      "qsub %0, %1, %2\n\t"
      : "=r"(res)
      : "r"(a), "r"(b)
  );
  return res;
}
#define silk_SUB_SAT32(a, b) (silk_SUB_SAT32_armv5e(a, b))

#undef silk_CLZ16
static inline opus_int32 silk_CLZ16_armv5(opus_int16 in16)
{
  int res;
  __asm__(
      "#silk_CLZ16\n\t"
      "clz %0, %1;\n"
      : "=r"(res)
      : "r"(in16<<16|0x8000)
  );
  return res;
}
#define silk_CLZ16(in16) (silk_CLZ16_armv5(in16))

#undef silk_CLZ32
static inline opus_int32 silk_CLZ32_armv5(opus_int32 in32)
{
  int res;
  __asm__(
      "#silk_CLZ32\n\t"
      "clz %0, %1\n\t"
      : "=r"(res)
      : "r"(in32)
  );
  return res;
}
#define silk_CLZ32(in32) (silk_CLZ32_armv5(in32))

#endif 
