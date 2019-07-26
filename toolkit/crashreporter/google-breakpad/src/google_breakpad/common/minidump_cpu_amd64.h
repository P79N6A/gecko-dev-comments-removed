







































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_AMD64_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_AMD64_H__






typedef struct {
  u_int16_t  control_word;
  u_int16_t  status_word;
  u_int8_t   tag_word;
  u_int8_t   reserved1;
  u_int16_t  error_opcode;
  u_int32_t  error_offset;
  u_int16_t  error_selector;
  u_int16_t  reserved2;
  u_int32_t  data_offset;
  u_int16_t  data_selector;
  u_int16_t  reserved3;
  u_int32_t  mx_csr;
  u_int32_t  mx_csr_mask;
  u_int128_t float_registers[8];
  u_int128_t xmm_registers[16];
  u_int8_t   reserved4[96];
} MDXmmSaveArea32AMD64;  

#define MD_CONTEXT_AMD64_VR_COUNT 26

typedef struct {
  


  u_int64_t  p1_home;
  u_int64_t  p2_home;
  u_int64_t  p3_home;
  u_int64_t  p4_home;
  u_int64_t  p5_home;
  u_int64_t  p6_home;

  

  u_int32_t  context_flags;
  u_int32_t  mx_csr;

  
  u_int16_t  cs;

  
  u_int16_t  ds;
  u_int16_t  es;
  u_int16_t  fs;
  u_int16_t  gs;

  
  u_int16_t  ss;
  u_int32_t  eflags;
  
  
  u_int64_t  dr0;
  u_int64_t  dr1;
  u_int64_t  dr2;
  u_int64_t  dr3;
  u_int64_t  dr6;
  u_int64_t  dr7;

  
  u_int64_t  rax;
  u_int64_t  rcx;
  u_int64_t  rdx;
  u_int64_t  rbx;

  
  u_int64_t  rsp;

  
  u_int64_t  rbp;
  u_int64_t  rsi;
  u_int64_t  rdi;
  u_int64_t  r8;
  u_int64_t  r9;
  u_int64_t  r10;
  u_int64_t  r11;
  u_int64_t  r12;
  u_int64_t  r13;
  u_int64_t  r14;
  u_int64_t  r15;

  
  u_int64_t  rip;

  


  union {
    MDXmmSaveArea32AMD64 flt_save;
    struct {
      u_int128_t header[2];
      u_int128_t legacy[8];
      u_int128_t xmm0;
      u_int128_t xmm1;
      u_int128_t xmm2;
      u_int128_t xmm3;
      u_int128_t xmm4;
      u_int128_t xmm5;
      u_int128_t xmm6;
      u_int128_t xmm7;
      u_int128_t xmm8;
      u_int128_t xmm9;
      u_int128_t xmm10;
      u_int128_t xmm11;
      u_int128_t xmm12;
      u_int128_t xmm13;
      u_int128_t xmm14;
      u_int128_t xmm15;
    } sse_registers;
  };

  u_int128_t vector_register[MD_CONTEXT_AMD64_VR_COUNT];
  u_int64_t  vector_control;

  
  u_int64_t debug_control;
  u_int64_t last_branch_to_rip;
  u_int64_t last_branch_from_rip;
  u_int64_t last_exception_to_rip;
  u_int64_t last_exception_from_rip;
  
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
