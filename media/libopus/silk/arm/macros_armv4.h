


























#ifndef SILK_MACROS_ARMv4_H
#define SILK_MACROS_ARMv4_H


#undef silk_SMULWB
static inline opus_int32 silk_SMULWB_armv4(opus_int32 a, opus_int16 b)
{
  unsigned rd_lo;
  int rd_hi;
  __asm__(
      "#silk_SMULWB\n\t"
      "smull %0, %1, %2, %3\n\t"
      : "=&r"(rd_lo), "=&r"(rd_hi)
      : "%r"(a), "r"(b<<16)
  );
  return rd_hi;
}
#define silk_SMULWB(a, b) (silk_SMULWB_armv4(a, b))


#undef silk_SMLAWB
#define silk_SMLAWB(a, b, c) ((a) + silk_SMULWB(b, c))


#undef silk_SMULWT
static inline opus_int32 silk_SMULWT_armv4(opus_int32 a, opus_int32 b)
{
  unsigned rd_lo;
  int rd_hi;
  __asm__(
      "#silk_SMULWT\n\t"
      "smull %0, %1, %2, %3\n\t"
      : "=&r"(rd_lo), "=&r"(rd_hi)
      : "%r"(a), "r"(b&~0xFFFF)
  );
  return rd_hi;
}
#define silk_SMULWT(a, b) (silk_SMULWT_armv4(a, b))


#undef silk_SMLAWT
#define silk_SMLAWT(a, b, c) ((a) + silk_SMULWT(b, c))


#undef silk_SMULWW
static inline opus_int32 silk_SMULWW_armv4(opus_int32 a, opus_int32 b)
{
  unsigned rd_lo;
  int rd_hi;
  __asm__(
    "#silk_SMULWW\n\t"
    "smull %0, %1, %2, %3\n\t"
    : "=&r"(rd_lo), "=&r"(rd_hi)
    : "%r"(a), "r"(b)
  );
  return (rd_hi<<16)+(rd_lo>>16);
}
#define silk_SMULWW(a, b) (silk_SMULWW_armv4(a, b))

#undef silk_SMLAWW
static inline opus_int32 silk_SMLAWW_armv4(opus_int32 a, opus_int32 b,
 opus_int32 c)
{
  unsigned rd_lo;
  int rd_hi;
  __asm__(
    "#silk_SMLAWW\n\t"
    "smull %0, %1, %2, %3\n\t"
    : "=&r"(rd_lo), "=&r"(rd_hi)
    : "%r"(b), "r"(c)
  );
  return a+(rd_hi<<16)+(rd_lo>>16);
}
#define silk_SMLAWW(a, b, c) (silk_SMLAWW_armv4(a, b, c))

#endif 
