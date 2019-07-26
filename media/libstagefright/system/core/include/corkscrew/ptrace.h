

















#ifndef _CORKSCREW_PTRACE_H
#define _CORKSCREW_PTRACE_H

#include <corkscrew/map_info.h>
#include <corkscrew/symbol_table.h>

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef struct {
    map_info_t* map_info_list;
} ptrace_context_t;


typedef struct {
    pid_t tid;
    const map_info_t* map_info_list;
} memory_t;

#if __i386__

typedef struct pt_regs_x86 {
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;
    uint32_t xds;
    uint32_t xes;
    uint32_t xfs;
    uint32_t xgs;
    uint32_t orig_eax;
    uint32_t eip;
    uint32_t xcs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t xss;
} pt_regs_x86_t;
#endif

#if __mips__

typedef struct pt_regs_mips {
    uint64_t regs[32];
    uint64_t lo;
    uint64_t hi;
    uint64_t cp0_epc;
    uint64_t cp0_badvaddr;
    uint64_t cp0_status;
    uint64_t cp0_cause;
} pt_regs_mips_t;
#endif




void init_memory(memory_t* memory, const map_info_t* map_info_list);





void init_memory_ptrace(memory_t* memory, pid_t tid);






bool try_get_word(const memory_t* memory, uintptr_t ptr, uint32_t* out_value);





bool try_get_word_ptrace(pid_t tid, uintptr_t ptr, uint32_t* out_value);











ptrace_context_t* load_ptrace_context(pid_t pid);




void free_ptrace_context(ptrace_context_t* context);






void find_symbol_ptrace(const ptrace_context_t* context,
        uintptr_t addr, const map_info_t** out_map_info, const symbol_t** out_symbol);

#ifdef __cplusplus
}
#endif

#endif
