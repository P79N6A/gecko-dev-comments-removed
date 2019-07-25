
























#ifndef _UPT_internal_h
#define _UPT_internal_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PTRACE_H
#include <sys/ptrace.h>
#endif
#ifdef HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif

#include <errno.h>
#include <libunwind-ptrace.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "libunwind_i.h"

struct UPT_info
  {
    pid_t pid;		
    struct elf_image ei;
    unw_dyn_info_t di_cache;
    unw_dyn_info_t di_debug;	
#if UNW_TARGET_IA64
    unw_dyn_info_t ktab;
#endif
#if UNW_TARGET_ARM
    unw_dyn_info_t di_arm;	
#endif
  };

extern int _UPT_reg_offset[UNW_REG_LAST + 1];

extern int _UPTi_find_unwind_table (struct UPT_info *ui,
				    unw_addr_space_t as,
				    char *path,
				    unw_word_t segbase,
				    unw_word_t mapoff,
				    unw_word_t ip);

#endif 
