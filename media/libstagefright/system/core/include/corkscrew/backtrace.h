

















#ifndef _CORKSCREW_BACKTRACE_H
#define _CORKSCREW_BACKTRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <corkscrew/ptrace.h>
#include <corkscrew/map_info.h>
#include <corkscrew/symbol_table.h>




typedef struct {
    uintptr_t absolute_pc;     
    uintptr_t stack_top;       
    size_t stack_size;         
} backtrace_frame_t;




typedef struct {
    uintptr_t relative_pc;       

    uintptr_t relative_symbol_addr; 

    char* map_name;              
    char* symbol_name;           
    char* demangled_name;        
} backtrace_symbol_t;






ssize_t unwind_backtrace(backtrace_frame_t* backtrace, size_t ignore_depth, size_t max_depth);








ssize_t unwind_backtrace_thread(pid_t tid, backtrace_frame_t* backtrace,
        size_t ignore_depth, size_t max_depth);






ssize_t unwind_backtrace_ptrace(pid_t tid, const ptrace_context_t* context,
        backtrace_frame_t* backtrace, size_t ignore_depth, size_t max_depth);






void get_backtrace_symbols(const backtrace_frame_t* backtrace, size_t frames,
        backtrace_symbol_t* backtrace_symbols);






void get_backtrace_symbols_ptrace(const ptrace_context_t* context,
        const backtrace_frame_t* backtrace, size_t frames,
        backtrace_symbol_t* backtrace_symbols);




void free_backtrace_symbols(backtrace_symbol_t* backtrace_symbols, size_t frames);

enum {
    
    MAX_BACKTRACE_LINE_LENGTH = 800,
};




void format_backtrace_line(unsigned frameNumber, const backtrace_frame_t* frame,
        const backtrace_symbol_t* symbol, char* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif
