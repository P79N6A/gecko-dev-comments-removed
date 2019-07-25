
























#ifndef RegexJIT_h
#define RegexJIT_h

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "RegexPattern.h"
#include "yarr/jswtfbridge.h"

#include "yarr/pcre/pcre.h"
struct JSRegExp; 

#if WTF_CPU_X86 && !WTF_COMPILER_MSVC
#define YARR_CALL __attribute__ ((regparm (3)))
#else
#define YARR_CALL
#endif

struct JSContext;

namespace JSC {

namespace Yarr {

class RegexCodeBlock {
    typedef int (*RegexJITCode)(const UChar* input, unsigned start, unsigned length, int* output) YARR_CALL;

public:
    RegexCodeBlock()
        : m_fallback(0)
    {
    }

    ~RegexCodeBlock()
    {
        if (m_fallback)
            jsRegExpFree(m_fallback);
        if (m_ref.m_size)
            m_ref.m_executablePool->release();
    }

    JSRegExp* getFallback() { return m_fallback; }
    void setFallback(JSRegExp* fallback) { m_fallback = fallback; }

    bool operator!() { return (!m_ref.m_code.executableAddress() && !m_fallback); }
    void set(MacroAssembler::CodeRef ref) { m_ref = ref; }

    int execute(const UChar* input, unsigned start, unsigned length, int* output)
    {
        return JS_EXTENSION(reinterpret_cast<RegexJITCode>(m_ref.m_code.executableAddress())(input, start, length, output));
    }

private:
    MacroAssembler::CodeRef m_ref;
    JSRegExp* m_fallback;
};

void jitCompileRegex(ExecutableAllocator &allocator, RegexCodeBlock& jitObject, const UString& pattern, unsigned& numSubpatterns, int& error, bool &fellBack, bool ignoreCase = false, bool multiline = false);

inline int executeRegex(JSContext *cx, RegexCodeBlock& jitObject, const UChar* input, unsigned start, unsigned length, int* output, int outputArraySize)
{
    if (JSRegExp* fallback = jitObject.getFallback())
        return (jsRegExpExecute(cx, fallback, input, length, start, output, outputArraySize) < 0 ? -1 : output[0]);

    return jitObject.execute(input, start, length, output);
}

} } 

#endif 
