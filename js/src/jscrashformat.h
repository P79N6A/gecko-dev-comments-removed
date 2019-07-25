







































#ifndef jscrashformat_h___
#define jscrashformat_h___

#include <string.h>

namespace js {
namespace crash {

const static int crash_cookie_len = 16;
const static char crash_cookie[crash_cookie_len] = "*J*S*CRASHDATA*";


enum {
    JS_CRASH_STACK_GC = 0x400,
    JS_CRASH_STACK_ERROR = 0x401,
    JS_CRASH_RING = 0x800
};







struct CrashHeader
{
    char cookie[crash_cookie_len];

    
    uint64 id;

    CrashHeader(uint64 id) : id(id) { memcpy(cookie, crash_cookie, crash_cookie_len); }
};

struct CrashRegisters
{
    uint64 ip, sp, bp;
};

const static int crash_buffer_size = 32 * 1024;

struct CrashStack
{
    CrashStack(uint64 id) : header(id) {}

    CrashHeader header;
    uint64 snaptime;      
    CrashRegisters regs;  
    uint64 stack_base;    
    uint64 stack_len;     
    char stack[crash_buffer_size]; 
};

struct CrashRing
{
    CrashRing(uint64 id) : header(id), offset(0) { memset(buffer, 0, sizeof(buffer)); }

    CrashHeader header;
    uint64 offset; 
    char buffer[crash_buffer_size];
};

} 
} 

#endif
