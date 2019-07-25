
































#ifndef COMMON_MAC_MACHO_UTILITIES_H__
#define COMMON_MAC_MACHO_UTILITIES_H__

#include <mach-o/loader.h>
#include <mach/thread_status.h>


#ifndef CPU_ARCH_ABI64
# define CPU_ARCH_ABI64    0x01000000
#endif

#ifndef CPU_TYPE_X86
# define CPU_TYPE_X86 CPU_TYPE_I386
#endif

#ifndef CPU_TYPE_POWERPC64
# define CPU_TYPE_POWERPC64 (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)
#endif

#ifndef LC_UUID
# define LC_UUID         0x1b    /* the uuid */
#endif

#if TARGET_CPU_X86
# define BREAKPAD_MACHINE_THREAD_STATE i386_THREAD_STATE
#elif TARGET_CPU_X86_64
# define BREAKPAD_MACHINE_THREAD_STATE x86_THREAD_STATE64
#else
# define BREAKPAD_MACHINE_THREAD_STATE MACHINE_THREAD_STATE
#endif



struct breakpad_uuid_command {
  uint32_t    cmd;            
  uint32_t    cmdsize;        
  uint8_t     uuid[16];       
};

void breakpad_swap_uuid_command(struct breakpad_uuid_command *uc,
                                enum NXByteOrder target_byte_order);



typedef natural_t breakpad_thread_state_data_t[THREAD_STATE_MAX];



void breakpad_swap_segment_command_64(struct segment_command_64 *sg,
                                      enum NXByteOrder target_byte_order);

void breakpad_swap_mach_header_64(struct mach_header_64 *mh,
                                  enum NXByteOrder target_byte_order);

void breakpad_swap_section_64(struct section_64 *s,
                              uint32_t nsects,
                              enum NXByteOrder target_byte_order);

#endif
