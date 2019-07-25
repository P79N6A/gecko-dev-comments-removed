#include "unwind_i.h"

static const char *regname[] =
  {
    
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    
    "r8",  "r9",  "r10", "fp",  "ip",  "sp",  "lr",  "pc",
    
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
    
    "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
    
    "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
    
    "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
    
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
    
    "wCGR0", "wCGR1", "wCGR2", "wCGR3", "wCGR4", "wCGR5", "wCGR6", "wCGR7",
    
    "wR0", "wR1", "wR2", "wR3", "wR4", "wR5", "wR6", "wR7",
    
    "spsr", "spsr_fiq", "spsr_irq", "spsr_abt", "spsr_und", "spsr_svc", 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    "r8_usr", "r9_usr", "r10_usr", "r11_usr", "r12_usr", "r13_usr", "r14_usr",
    
    "r8_fiq", "r9_fiq", "r10_fiq", "r11_fiq", "r12_fiq", "r13_fiq", "r14_fiq",
    
    "r13_irq", "r14_irq",
    
    "r13_abt", "r14_abt",
    
    "r13_und", "r14_und",
    
    "r13_svc", "r14_svc", 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    "wC0", "wC1", "wC2", "wC3", "wC4", "wC5", "wC6", "wC7",
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0,
    
    "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
    
    "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
    
    "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
    
    "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
  };

PROTECTED const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < (unw_regnum_t) ARRAY_SIZE (regname))
    return regname[reg];
  else
    return "???";
}
