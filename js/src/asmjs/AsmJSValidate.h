

















#ifndef jit_AsmJS_h
#define jit_AsmJS_h

#include "mozilla/MathAlgorithms.h"

#include <stddef.h>

#include "jsutil.h"

#include "jit/Registers.h"
#include "js/TypeDecls.h"
#include "vm/NativeObject.h"

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
ValidateAsmJS(ExclusiveContext* cx, AsmJSParser& parser, frontend::ParseNode* stmtList,
             bool* validated);


const size_t AsmJSPageSize = 4096;

#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)




static_assert(jit::AsmJSCheckedImmediateRange <= jit::AsmJSImmediateRange,
              "AsmJSImmediateRange should be the size of an unconstrained "
              "address immediate");









static const size_t AsmJSMappedSize = 4 * 1024ULL * 1024ULL * 1024ULL +
                                      jit::AsmJSCheckedImmediateRange +
                                      AsmJSPageSize;

#endif 







inline uint32_t
RoundUpToNextValidAsmJSHeapLength(uint32_t length)
{
    if (length <= 4 * 1024)
        return 4 * 1024;

    if (length <= 16 * 1024 * 1024)
        return mozilla::RoundUpPow2(length);

    MOZ_ASSERT(length <= 0xff000000);
    return (length + 0x00ffffff) & ~0x00ffffff;
}

inline bool
IsValidAsmJSHeapLength(uint32_t length)
{
    bool valid = length >= 4 * 1024 &&
                 (IsPowerOfTwo(length) ||
                  (length & 0x00ffffff) == 0);

    MOZ_ASSERT_IF(valid, length % AsmJSPageSize == 0);
    MOZ_ASSERT_IF(valid, length == RoundUpToNextValidAsmJSHeapLength(length));

    return valid;
}



inline bool
IsDeprecatedAsmJSHeapLength(uint32_t length)
{
    return length >= 4 * 1024 && length < 64 * 1024 && IsPowerOfTwo(length);
}



extern bool
IsAsmJSCompilationAvailable(JSContext* cx, unsigned argc, JS::Value* vp);

} 

#endif 
