





































#ifndef SRC_CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_EXTENSION_LINUX_H_
#define SRC_CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_EXTENSION_LINUX_H_

#include <stddef.h>

#include "google_breakpad/common/breakpad_types.h"



enum {
  MD_LINUX_CPU_INFO              = 0x47670003,    
  MD_LINUX_PROC_STATUS           = 0x47670004,    
  MD_LINUX_LSB_RELEASE           = 0x47670005,    
  MD_LINUX_CMD_LINE              = 0x47670006,    
  MD_LINUX_ENVIRON               = 0x47670007,    
  MD_LINUX_AUXV                  = 0x47670008,    
  MD_LINUX_MAPS                  = 0x47670009,    
  MD_LINUX_DSO_DEBUG             = 0x4767000A     
};

typedef struct {
  void*     addr;
  MDRVA     name;
  void*     ld;
} MDRawLinkMap;

typedef struct {
  u_int32_t version;
  MDRVA     map;
  u_int32_t dso_count;
  void*     brk;
  void*     ldbase;
  void*     dynamic;
} MDRawDebug;

#endif  
