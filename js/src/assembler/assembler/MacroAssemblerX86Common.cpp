





#include "assembler/assembler/MacroAssemblerX86Common.h"

#ifdef _MSC_VER
#ifdef JS_CODEGEN_X64

#include <intrin.h>
#endif
#endif

using namespace JSC;
MacroAssemblerX86Common::SSECheckState MacroAssemblerX86Common::s_sseCheckState = NotCheckedSSE;

#ifdef DEBUG
bool MacroAssemblerX86Common::s_floatingPointDisabled = false;
bool MacroAssemblerX86Common::s_SSE3Disabled = false;
bool MacroAssemblerX86Common::s_SSE4Disabled = false;
#endif

void MacroAssemblerX86Common::setSSECheckState()
{
    
    
    int flags_edx = 0;
    int flags_ecx = 0;
#ifdef _MSC_VER
#ifdef JS_CODEGEN_X64
    int cpuinfo[4];

    __cpuid(cpuinfo, 1);
    flags_ecx = cpuinfo[2];
    flags_edx = cpuinfo[3];
#else
    _asm {
        mov eax, 1 
        cpuid;
        mov flags_ecx, ecx;
        mov flags_edx, edx;
    }
#endif
#elif defined(__GNUC__)
#ifdef JS_CODEGEN_X64
    asm (
         "movl $0x1, %%eax;"
         "cpuid;"
         : "=c" (flags_ecx), "=d" (flags_edx)
         :
         : "%eax", "%ebx"
         );
#else
    
    asm (
         "movl $0x1, %%eax;"
         "pushl %%ebx;"
         "cpuid;"
         "popl %%ebx;"
         : "=c" (flags_ecx), "=d" (flags_edx)
         :
         : "%eax"
         );
#endif
#endif

#ifdef DEBUG
    if (s_floatingPointDisabled) {
        
        s_sseCheckState = HasSSE;
        return;
    }
#endif

    static const int SSEFeatureBit = 1 << 25;
    static const int SSE2FeatureBit = 1 << 26;
    static const int SSE3FeatureBit = 1 << 0;
    static const int SSSE3FeatureBit = 1 << 9;
    static const int SSE41FeatureBit = 1 << 19;
    static const int SSE42FeatureBit = 1 << 20;
    if (flags_ecx & SSE42FeatureBit)
        s_sseCheckState = HasSSE4_2;
    else if (flags_ecx & SSE41FeatureBit)
        s_sseCheckState = HasSSE4_1;
    else if (flags_ecx & SSSE3FeatureBit)
        s_sseCheckState = HasSSSE3;
    else if (flags_ecx & SSE3FeatureBit)
        s_sseCheckState = HasSSE3;
    else if (flags_edx & SSE2FeatureBit)
        s_sseCheckState = HasSSE2;
    else if (flags_edx & SSEFeatureBit)
        s_sseCheckState = HasSSE;
    else
        s_sseCheckState = NoSSE;

#ifdef DEBUG
    if (s_sseCheckState >= HasSSE4_1 && s_SSE4Disabled)
        s_sseCheckState = HasSSE3;
    if (s_sseCheckState >= HasSSE3 && s_SSE3Disabled)
        s_sseCheckState = HasSSE2;
#endif
}
