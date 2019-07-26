

























#ifndef FIXED_ARMv4_H
#define FIXED_ARMv4_H


#undef MULT16_32_Q16
static inline opus_val32 MULT16_32_Q16_armv4(opus_val16 a, opus_val32 b)
{
  unsigned rd_lo;
  int rd_hi;
  __asm__(
      "#MULT16_32_Q16\n\t"
      "smull %0, %1, %2, %3\n\t"
      : "=&r"(rd_lo), "=&r"(rd_hi)
      : "%r"(b),"r"(a<<16)
  );
  return rd_hi;
}
#define MULT16_32_Q16(a, b) (MULT16_32_Q16_armv4(a, b))



#undef MULT16_32_Q15
static inline opus_val32 MULT16_32_Q15_armv4(opus_val16 a, opus_val32 b)
{
  unsigned rd_lo;
  int rd_hi;
  __asm__(
      "#MULT16_32_Q15\n\t"
      "smull %0, %1, %2, %3\n\t"
      : "=&r"(rd_lo), "=&r"(rd_hi)
      : "%r"(b), "r"(a<<16)
  );
  
  return rd_hi<<1;
}
#define MULT16_32_Q15(a, b) (MULT16_32_Q15_armv4(a, b))





#undef MAC16_32_Q15
#define MAC16_32_Q15(c, a, b) ADD32(c, MULT16_32_Q15(a, b))



#undef MULT32_32_Q31
#define MULT32_32_Q31(a,b) (opus_val32)((((opus_int64)(a)) * ((opus_int64)(b)))>>31)

#endif
