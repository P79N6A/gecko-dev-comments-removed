





#ifndef jit_AsmJS_h
#define jit_AsmJS_h

#include <stddef.h>

#include "js/TypeDecls.h"
#include "vm/ObjectImpl.h"

namespace js {

class ExclusiveContext;
namespace frontend {
    template <typename ParseHandler> struct Parser;
    template <typename ParseHandler> struct ParseContext;
    class FullParseHandler;
    struct ParseNode;
}

typedef frontend::Parser<frontend::FullParseHandler> AsmJSParser;
typedef frontend::ParseContext<frontend::FullParseHandler> AsmJSParseContext;






extern bool
CompileAsmJS(ExclusiveContext *cx, AsmJSParser &parser, frontend::ParseNode *stmtList,
             bool *validated);


const size_t AsmJSPageSize = 4096;


static const size_t AsmJSAllocationGranularity = 4096;

#ifdef JS_CODEGEN_X64



static const size_t AsmJSBufferProtectedSize = 4 * 1024ULL * 1024ULL * 1024ULL;





















static const size_t AsmJSMappedSize = AsmJSPageSize + AsmJSBufferProtectedSize;
#endif 

#ifdef JS_ION



extern bool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, JS::Value *vp);

#else 

inline bool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().set(BooleanValue(false));
    return true;
}

#endif 





























inline uint32_t
RoundUpToNextValidAsmJSHeapLength(uint32_t length)
{
    if (length < 0x00001000u) 
        return 0x1000u;
    if (length < 0x00100000u) 
        return (length + 0x00000fff) & ~0x00000fff;
    if (length < 0x00400000u) 
        return (length + 0x00003fff) & ~0x00003fff;
    if (length < 0x01000000u) 
        return (length + 0x0000ffff) & ~0x0000ffff;
    if (length < 0x04000000u) 
        return (length + 0x0003ffff) & ~0x0003ffff;
    if (length < 0x10000000u) 
        return (length + 0x000fffff) & ~0x000fffff;
    if (length < 0x40000000u) 
        return (length + 0x003fffff) & ~0x003fffff;
    
    
    JS_ASSERT(length <= 0xff000000);
    return (length + 0x00ffffff) & ~0x00ffffff;
}

inline bool
IsValidAsmJSHeapLength(uint32_t length)
{
    if (length <  AsmJSAllocationGranularity)
        return false;
    if (length <= 0x00100000u)
        return (length & 0x00000fff) == 0;
    if (length <= 0x00400000u)
        return (length & 0x00003fff) == 0;
    if (length <= 0x01000000u)
        return (length & 0x0000ffff) == 0;
    if (length <= 0x04000000u)
        return (length & 0x0003ffff) == 0;
    if (length <= 0x10000000u)
        return (length & 0x000fffff) == 0;
    if (length <= 0x40000000u)
        return (length & 0x003fffff) == 0;
    if (length <= 0xff000000u)
        return (length & 0x00ffffff) == 0;
    return false;
}

} 

#endif 
