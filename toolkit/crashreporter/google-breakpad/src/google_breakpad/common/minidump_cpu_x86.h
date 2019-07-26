






































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_X86_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_X86_H__

#define MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE 80
     

typedef struct {
  uint32_t control_word;
  uint32_t status_word;
  uint32_t tag_word;
  uint32_t error_offset;
  uint32_t error_selector;
  uint32_t data_offset;
  uint32_t data_selector;

  

  uint8_t  register_area[MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE];
  uint32_t cr0_npx_state;
} MDFloatingSaveAreaX86;  


#define MD_CONTEXT_X86_EXTENDED_REGISTERS_SIZE 512
     

typedef struct {
  

  uint32_t             context_flags;

  
  uint32_t             dr0;
  uint32_t             dr1;
  uint32_t             dr2;
  uint32_t             dr3;
  uint32_t             dr6;
  uint32_t             dr7;

  
  MDFloatingSaveAreaX86 float_save;

  
  uint32_t             gs; 
  uint32_t             fs;
  uint32_t             es;
  uint32_t             ds;
  
  uint32_t             edi;
  uint32_t             esi;
  uint32_t             ebx;
  uint32_t             edx;
  uint32_t             ecx;
  uint32_t             eax;

  
  uint32_t             ebp;
  uint32_t             eip;
  uint32_t             cs;      
  uint32_t             eflags;  
  uint32_t             esp;
  uint32_t             ss;

  




  uint8_t              extended_registers[
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
