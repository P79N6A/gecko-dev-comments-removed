







































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_AMD64_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_AMD64_H__






typedef struct {
  uint16_t  control_word;
  uint16_t  status_word;
  uint8_t   tag_word;
  uint8_t   reserved1;
  uint16_t  error_opcode;
  uint32_t  error_offset;
  uint16_t  error_selector;
  uint16_t  reserved2;
  uint32_t  data_offset;
  uint16_t  data_selector;
  uint16_t  reserved3;
  uint32_t  mx_csr;
  uint32_t  mx_csr_mask;
  uint128_t float_registers[8];
  uint128_t xmm_registers[16];
  uint8_t   reserved4[96];
} MDXmmSaveArea32AMD64;  

#define MD_CONTEXT_AMD64_VR_COUNT 26

typedef struct {
  


  uint64_t  p1_home;
  uint64_t  p2_home;
  uint64_t  p3_home;
  uint64_t  p4_home;
  uint64_t  p5_home;
  uint64_t  p6_home;

  

  uint32_t  context_flags;
  uint32_t  mx_csr;

  
  uint16_t  cs;

  
  uint16_t  ds;
  uint16_t  es;
  uint16_t  fs;
  uint16_t  gs;

  
  uint16_t  ss;
  uint32_t  eflags;
  
  
  uint64_t  dr0;
  uint64_t  dr1;
  uint64_t  dr2;
  uint64_t  dr3;
  uint64_t  dr6;
  uint64_t  dr7;

  
  uint64_t  rax;
  uint64_t  rcx;
  uint64_t  rdx;
  uint64_t  rbx;

  
  uint64_t  rsp;

  
  uint64_t  rbp;
  uint64_t  rsi;
  uint64_t  rdi;
  uint64_t  r8;
  uint64_t  r9;
  uint64_t  r10;
  uint64_t  r11;
  uint64_t  r12;
  uint64_t  r13;
  uint64_t  r14;
  uint64_t  r15;

  
  uint64_t  rip;

  


  union {
    MDXmmSaveArea32AMD64 flt_save;
    struct {
      uint128_t header[2];
      uint128_t legacy[8];
      uint128_t xmm0;
      uint128_t xmm1;
      uint128_t xmm2;
      uint128_t xmm3;
      uint128_t xmm4;
      uint128_t xmm5;
      uint128_t xmm6;
      uint128_t xmm7;
      uint128_t xmm8;
      uint128_t xmm9;
      uint128_t xmm10;
      uint128_t xmm11;
      uint128_t xmm12;
      uint128_t xmm13;
      uint128_t xmm14;
      uint128_t xmm15;
    } sse_registers;
  };

  uint128_t vector_register[MD_CONTEXT_AMD64_VR_COUNT];
  uint64_t  vector_control;

  
  uint64_t debug_control;
  uint64_t last_branch_to_rip;
  uint64_t last_branch_from_rip;
  uint64_t last_exception_to_rip;
  uint64_t last_exception_from_rip;
  
} MDRawContextAMD64;  




#define MD_CONTEXT_AMD64 0x00100000  /* CONTEXT_AMD64 */
#define MD_CONTEXT_AMD64_CONTROL         (MD_CONTEXT_AMD64 | 0x00000001)
     
#define MD_CONTEXT_AMD64_INTEGER         (MD_CONTEXT_AMD64 | 0x00000002)
     
#define MD_CONTEXT_AMD64_SEGMENTS        (MD_CONTEXT_AMD64 | 0x00000004)
     
#define MD_CONTEXT_AMD64_FLOATING_POINT  (MD_CONTEXT_AMD64 | 0x00000008)
     
#define MD_CONTEXT_AMD64_DEBUG_REGISTERS (MD_CONTEXT_AMD64 | 0x00000010)
     
#define MD_CONTEXT_AMD64_XSTATE          (MD_CONTEXT_AMD64 | 0x00000040)
     





#define MD_CONTEXT_AMD64_FULL            (MD_CONTEXT_AMD64_CONTROL | \
                                          MD_CONTEXT_AMD64_INTEGER | \
                                          MD_CONTEXT_AMD64_FLOATING_POINT)
     

#define MD_CONTEXT_AMD64_ALL             (MD_CONTEXT_AMD64_FULL | \
                                          MD_CONTEXT_AMD64_SEGMENTS | \
                                          MD_CONTEXT_X86_DEBUG_REGISTERS)
     


#endif 
