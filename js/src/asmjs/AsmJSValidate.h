





#ifndef jit_AsmJS_h
#define jit_AsmJS_h

#include "mozilla/MathAlgorithms.h"

#include <stddef.h>

#include "jsutil.h"

#include "js/TypeDecls.h"
#include "vm/ObjectImpl.h"

namespace js {

class ExclusiveContext;
namespace frontend {
    template <typename ParseHandler> class Parser;
    template <typename ParseHandler> struct ParseContext;
    class FullParseHandler;
    class ParseNode;
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



extern bool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, JS::Value *vp);





inline bool
IsValidAsmJSHeapLength(uint32_t length)
{
    if (length < 4096)
        return false;

    if (IsPowerOfTwo(length))
        return true;

    return (length & 0x00ffffff) == 0;
}

inline uint32_t
RoundUpToNextValidAsmJSHeapLength(uint32_t length)
{
    if (length < 4096)
        return 4096;

    if (length < 16 * 1024 * 1024)
        return mozilla::RoundUpPow2(length);

    JS_ASSERT(length <= 0xff000000);
    return (length + 0x00ffffff) & ~0x00ffffff;
}

} 

#endif 
