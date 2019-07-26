


























#ifndef yarr_YarrJIT_h
#define yarr_YarrJIT_h

#include "assembler/wtf/Platform.h"

#if JS_ION

#include "assembler/assembler/MacroAssemblerCodeRef.h"

#include "yarr/MatchResult.h"
#include "yarr/Yarr.h"

#if WTF_CPU_X86 && !WTF_COMPILER_MSVC && !WTF_COMPILER_SUNCC
#define YARR_CALL __attribute__ ((regparm (3)))
#else
#define YARR_CALL
#endif

#include "jit/JitCommon.h"

namespace JSC {

class JSGlobalData;
class ExecutablePool;

namespace Yarr {

class YarrCodeBlock {
#if defined(WTF_CPU_X86_64) && !defined(_WIN64)
    typedef MatchResult JITMatchResult;
#else
    typedef EncodedMatchResult JITMatchResult;
#endif

    typedef JITMatchResult (*YarrJITCode8)(const LChar* input, unsigned start, unsigned length, int* output) YARR_CALL;
    typedef JITMatchResult (*YarrJITCode16)(const UChar* input, unsigned start, unsigned length, int* output) YARR_CALL;
    typedef JITMatchResult (*YarrJITCodeMatchOnly8)(const LChar* input, unsigned start, unsigned length) YARR_CALL;
    typedef JITMatchResult (*YarrJITCodeMatchOnly16)(const UChar* input, unsigned start, unsigned length) YARR_CALL;

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

#ifdef YARR_8BIT_CHAR_SUPPORT
    bool has8BitCode() const { return m_ref8.allocSize(); }
    void set8BitCode(MacroAssemblerCodeRef ref) { m_ref8 = ref; }
    bool has8BitCodeMatchOnly() const { return m_matchOnly8.allocSize(); }
    void set8BitCodeMatchOnly(MacroAssemblerCodeRef matchOnly) { m_matchOnly8 = matchOnly; }
#endif

    bool has16BitCode() const { return m_ref16.allocSize(); }
    void set16BitCode(MacroAssemblerCodeRef ref) { m_ref16 = ref; }

    bool has16BitCodeMatchOnly() const { return m_matchOnly16.allocSize(); }
    void set16BitCodeMatchOnly(MacroAssemblerCodeRef matchOnly) { m_matchOnly16 = matchOnly; }

#if YARR_8BIT_CHAR_SUPPORT
    MatchResult execute(const LChar* input, unsigned start, unsigned length, int* output)
    {
        ASSERT(has8BitCode());

        return MatchResult(reinterpret_cast<YarrJITCode8>(m_ref8.code().executableAddress())(input, start, length, output));
    }

    MatchResult execute(const LChar* input, unsigned start, unsigned length)
    {
        ASSERT(has8BitCodeMatchOnly());

        return MatchResult(reinterpret_cast<YarrJITCodeMatchOnly8>(m_matchOnly8.code().executableAddress())(input, start, length));
    }
#endif

    MatchResult execute(const UChar* input, unsigned start, unsigned length, int* output)
    {
        ASSERT(has16BitCode());

        YarrJITCode16 fn = JS_FUNC_TO_DATA_PTR(YarrJITCode16, m_ref16.code().executableAddress());
        return MatchResult(CALL_GENERATED_YARR_CODE4(fn, input, start, length, output));
    }

    MatchResult execute(const UChar* input, unsigned start, unsigned length)
    {
        ASSERT(has16BitCodeMatchOnly());

        YarrJITCodeMatchOnly16 fn = JS_FUNC_TO_DATA_PTR(YarrJITCodeMatchOnly16, m_matchOnly16.code().executableAddress());
        return MatchResult(CALL_GENERATED_YARR_CODE3(fn, input, start, length));
    }

#if ENABLE_REGEXP_TRACING
    void *getAddr() { return m_ref.code().executableAddress(); }
#endif

    void clear()
    {
#ifdef YARR_8BIT_CHAR_SUPPORT
        m_ref8 = MacroAssemblerCodeRef();
        m_matchOnly8 = MacroAssemblerCodeRef();
#endif

        m_ref16 = MacroAssemblerCodeRef();
        m_matchOnly16 = MacroAssemblerCodeRef();
        m_needFallBack = false;
    }

    void release() {
#ifdef YARR_8BIT_CHAR_SUPPORT
        m_ref8.release();
        m_matchOnly8.release();
#endif

        m_ref16.release();
        m_matchOnly16.release();
    }

private:
#ifdef YARR_8BIT_CHAR_SUPPORT
    MacroAssemblerCodeRef m_ref8;
    MacroAssemblerCodeRef m_matchOnly8;
#endif

    MacroAssemblerCodeRef m_ref16;
    MacroAssemblerCodeRef m_matchOnly16;
    bool m_needFallBack;
};

enum YarrJITCompileMode {
    MatchOnly,
    IncludeSubpatterns
};
void jitCompile(YarrPattern&, YarrCharSize, JSGlobalData*, YarrCodeBlock& jitObject, YarrJITCompileMode = IncludeSubpatterns);

} } 

#endif

#endif 
