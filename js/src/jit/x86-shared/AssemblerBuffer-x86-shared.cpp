





#include "jit/x86-shared/AssemblerBuffer-x86-shared.h"

#include "jsopcode.h"

void js::jit::GenericAssembler::spew(const char* fmt, va_list va)
{
    
    
    char buf[200];

    int i = vsnprintf(buf, sizeof(buf), fmt, va);

    if (i > -1) {
        if (printer)
            printer->printf("%s\n", buf);
        js::jit::JitSpew(js::jit::JitSpew_Codegen, "%s", buf);
    }
}
