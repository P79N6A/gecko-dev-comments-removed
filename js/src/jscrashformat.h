





#ifndef jscrashformat_h
#define jscrashformat_h

namespace js {
namespace crash {

static const int crash_cookie_len = 16;
static const char crash_cookie[crash_cookie_len] = "*J*S*CRASHDATA*";


enum {
    JS_CRASH_STACK_GC = 0x400,
    JS_CRASH_STACK_ERROR = 0x401,
    JS_CRASH_RING = 0x800
};







struct CrashHeader
{
    char cookie[crash_cookie_len];

    
    uint64_t id;

    explicit CrashHeader(uint64_t id) : id(id) { memcpy(cookie, crash_cookie, crash_cookie_len); }
};

struct CrashRegisters
{
    uint64_t ip, sp, bp;
};

static const int crash_buffer_size = 32 * 1024;

struct CrashStack
{
    explicit CrashStack(uint64_t id) : header(id) {}

    CrashHeader header;
    uint64_t snaptime;    
    CrashRegisters regs;  
    uint64_t stack_base;  
    uint64_t stack_len;   
    char stack[crash_buffer_size]; 
};

struct CrashRing
{
    explicit CrashRing(uint64_t id) : header(id), offset(0) { memset(buffer, 0, sizeof(buffer)); }

    CrashHeader header;
    uint64_t offset; 
    char buffer[crash_buffer_size];
};


enum {
    JS_CRASH_TAG_GC = 0x200
};

} 
} 

#endif 
