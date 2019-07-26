






































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_X86_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_X86_H__

#define MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE 80
     

typedef struct {
  u_int32_t control_word;
  u_int32_t status_word;
  u_int32_t tag_word;
  u_int32_t error_offset;
  u_int32_t error_selector;
  u_int32_t data_offset;
  u_int32_t data_selector;

  

  u_int8_t  register_area[MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE];
  u_int32_t cr0_npx_state;
} MDFloatingSaveAreaX86;  


#define MD_CONTEXT_X86_EXTENDED_REGISTERS_SIZE 512
     

typedef struct {
  

  u_int32_t             context_flags;

  
  u_int32_t             dr0;
  u_int32_t             dr1;
  u_int32_t             dr2;
  u_int32_t             dr3;
  u_int32_t             dr6;
  u_int32_t             dr7;

  
  MDFloatingSaveAreaX86 float_save;

  
  u_int32_t             gs; 
  u_int32_t             fs;
  u_int32_t             es;
  u_int32_t             ds;
  
  u_int32_t             edi;
  u_int32_t             esi;
  u_int32_t             ebx;
  u_int32_t             edx;
  u_int32_t             ecx;
  u_int32_t             eax;

  
  u_int32_t             ebp;
  u_int32_t             eip;
  u_int32_t             cs;      
  u_int32_t             eflags;  
  u_int32_t             esp;
  u_int32_t             ss;

  




  u_int8_t              extended_registers[
                         MD_CONTEXT_X86_EXTENDED_REGISTERS_SIZE];
} MDRawContextX86;  




#define MD_CONTEXT_X86                    0x00010000
     
#define MD_CONTEXT_X86_CONTROL            (MD_CONTEXT_X86 | 0x00000001)
     
#define MD_CONTEXT_X86_INTEGER            (MD_CONTEXT_X86 | 0x00000002)
     
#define MD_CONTEXT_X86_SEGMENTS           (MD_CONTEXT_X86 | 0x00000004)
     
#define MD_CONTEXT_X86_FLOATING_POINT     (MD_CONTEXT_X86 | 0x00000008)
     
#define MD_CONTEXT_X86_DEBUG_REGISTERS    (MD_CONTEXT_X86 | 0x00000010)
     
#define MD_CONTEXT_X86_EXTENDED_REGISTERS (MD_CONTEXT_X86 | 0x00000020)
     
#define MD_CONTEXT_X86_XSTATE             (MD_CONTEXT_X86 | 0x00000040)
     

#define MD_CONTEXT_X86_FULL              (MD_CONTEXT_X86_CONTROL | \
                                          MD_CONTEXT_X86_INTEGER | \
                                          MD_CONTEXT_X86_SEGMENTS)
     

#define MD_CONTEXT_X86_ALL               (MD_CONTEXT_X86_FULL | \
                                          MD_CONTEXT_X86_FLOATING_POINT | \
                                          MD_CONTEXT_X86_DEBUG_REGISTERS | \
                                          MD_CONTEXT_X86_EXTENDED_REGISTERS)
     

#endif 
