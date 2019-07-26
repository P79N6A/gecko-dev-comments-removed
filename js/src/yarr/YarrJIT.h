




























#ifndef YarrJIT_h
#define YarrJIT_h

#include "assembler/wtf/Platform.h"

#if ENABLE_YARR_JIT

#include "assembler/assembler/MacroAssembler.h"
#include "YarrPattern.h"

#if WTF_CPU_X86 && !WTF_COMPILER_MSVC && !WTF_COMPILER_SUNCC
#define YARR_CALL __attribute__ ((regparm (3)))
#else
#define YARR_CALL
#endif

#if JS_TRACE_LOGGING
#include "TraceLogging.h"
#endif

namespace JSC {

class JSGlobalData;
class ExecutablePool;

namespace Yarr {

class YarrCodeBlock {
    typedef int (*YarrJITCode)(const UChar* input, unsigned start, unsigned length, int* output) YARR_CALL;

public:
    YarrCodeBlock()
        : m_needFallBack(false)
    {
    }

    ~YarrCodeBlock()
    {
    }

    void setFallBack(bool fallback) { m_needFallBack = fallback; }
    bool isFallBack() { return m_needFallBack; }
    void set(MacroAssembler::CodeRef ref) { m_ref = ref; }

    int execute(const UChar* input, unsigned start, unsigned length, int* output)
    {
#if JS_TRACE_LOGGING
        js::AutoTraceLog logger(js::TraceLogging::defaultLogger(),
                                js::TraceLogging::YARR_YIT_START,
                                js::TraceLogging::YARR_YIT_STOP);
#endif

        return JS_EXTENSION((reinterpret_cast<YarrJITCode>(m_ref.m_code.executableAddress()))(input, start, length, output));
    }

#if ENABLE_REGEXP_TRACING
    void *getAddr() { return m_ref.m_code.executableAddress(); }
#endif

    void release() { m_ref.release(); }

private:
    MacroAssembler::CodeRef m_ref;
    bool m_needFallBack;
};

void jitCompile(YarrPattern&, JSGlobalData*, YarrCodeBlock& jitObject);
int execute(YarrCodeBlock& jitObject, const UChar* input, unsigned start, unsigned length, int* output);

} } 

#endif

#endif 
