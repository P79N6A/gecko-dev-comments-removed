











































#if !defined(_G711_H_)
#define _G711_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "typedefs.h"

#if defined(__i386__)



static __inline__ int top_bit(unsigned int bits) {
  int res;

  __asm__ __volatile__(" movl $-1,%%edx;\n"
                       " bsrl %%eax,%%edx;\n"
                       : "=d" (res)
                       : "a" (bits));
  return res;
}




static __inline__ int bottom_bit(unsigned int bits) {
  int res;

  __asm__ __volatile__(" movl $-1,%%edx;\n"
                       " bsfl %%eax,%%edx;\n"
                       : "=d" (res)
                       : "a" (bits));
  return res;
}
#elif defined(__x86_64__)
static __inline__ int top_bit(unsigned int bits) {
  int res;

  __asm__ __volatile__(" movq $-1,%%rdx;\n"
                       " bsrq %%rax,%%rdx;\n"
                       : "=d" (res)
                       : "a" (bits));
  return res;
}

static __inline__ int bottom_bit(unsigned int bits) {
  int res;

  __asm__ __volatile__(" movq $-1,%%rdx;\n"
                       " bsfq %%rax,%%rdx;\n"
                       : "=d" (res)
                       : "a" (bits));
  return res;
}
#else
static __inline int top_bit(unsigned int bits) {
  int i;

  if (bits == 0) {
    return -1;
  }
  i = 0;
  if (bits & 0xFFFF0000) {
    bits &= 0xFFFF0000;
    i += 16;
  }
  if (bits & 0xFF00FF00) {
    bits &= 0xFF00FF00;
    i += 8;
  }
  if (bits & 0xF0F0F0F0) {
    bits &= 0xF0F0F0F0;
    i += 4;
  }
  if (bits & 0xCCCCCCCC) {
    bits &= 0xCCCCCCCC;
    i += 2;
  }
  if (bits & 0xAAAAAAAA) {
    bits &= 0xAAAAAAAA;
    i += 1;
  }
  return i;
}

static __inline int bottom_bit(unsigned int bits) {
  int i;

  if (bits == 0) {
    return -1;
  }
  i = 32;
  if (bits & 0x0000FFFF) {
    bits &= 0x0000FFFF;
    i -= 16;
  }
  if (bits & 0x00FF00FF) {
    bits &= 0x00FF00FF;
    i -= 8;
  }
  if (bits & 0x0F0F0F0F) {
    bits &= 0x0F0F0F0F;
    i -= 4;
  }
  if (bits & 0x33333333) {
    bits &= 0x33333333;
    i -= 2;
  }
  if (bits & 0x55555555) {
    bits &= 0x55555555;
    i -= 1;
  }
  return i;
}
#endif







































#define ULAW_BIAS 0x84  /* Bias for linear code. */





static __inline uint8_t linear_to_ulaw(int linear) {
  uint8_t u_val;
  int mask;
  int seg;

  
  if (linear < 0) {
    
    linear = ULAW_BIAS - linear - 1;
    mask = 0x7F;
  } else {
    linear = ULAW_BIAS + linear;
    mask = 0xFF;
  }

  seg = top_bit(linear | 0xFF) - 7;

  



  if (seg >= 8)
    u_val = (uint8_t)(0x7F ^ mask);
  else
    u_val = (uint8_t)(((seg << 4) | ((linear >> (seg + 3)) & 0xF)) ^ mask);
#ifdef ULAW_ZEROTRAP
  
  if (u_val == 0)
    u_val = 0x02;
#endif
  return u_val;
}





static __inline int16_t ulaw_to_linear(uint8_t ulaw) {
  int t;

  
  ulaw = ~ulaw;
  



  t = (((ulaw & 0x0F) << 3) + ULAW_BIAS) << (((int) ulaw & 0x70) >> 4);
  return (int16_t)((ulaw & 0x80) ? (ULAW_BIAS - t) : (t - ULAW_BIAS));
}



















#define ALAW_AMI_MASK 0x55





static __inline uint8_t linear_to_alaw(int linear) {
  int mask;
  int seg;

  if (linear >= 0) {
    
    mask = ALAW_AMI_MASK | 0x80;
  } else {
    
    mask = ALAW_AMI_MASK;
    

    linear = -linear - 1;
  }

  
  seg = top_bit(linear | 0xFF) - 7;
  if (seg >= 8) {
    if (linear >= 0) {
      
      return (uint8_t)(0x7F ^ mask);
    }
    
    return (uint8_t)(0x00 ^ mask);
  }
  
  return (uint8_t)(((seg << 4) | ((linear >> ((seg) ? (seg + 3) : 4)) & 0x0F)) ^
                   mask);
}





static __inline int16_t alaw_to_linear(uint8_t alaw) {
  int i;
  int seg;

  alaw ^= ALAW_AMI_MASK;
  i = ((alaw & 0x0F) << 4);
  seg = (((int) alaw & 0x70) >> 4);
  if (seg)
    i = (i + 0x108) << (seg - 1);
  else
    i += 8;
  return (int16_t)((alaw & 0x80) ? i : -i);
}





uint8_t alaw_to_ulaw(uint8_t alaw);





uint8_t ulaw_to_alaw(uint8_t ulaw);

#ifdef __cplusplus
}
#endif

#endif
