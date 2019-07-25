





































#include "MethodJIT.h"
#include "Logging.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "jstracer.h"
#include "BaseAssembler.h"
#include "Compiler.h"
#include "MonoIC.h"
#include "PolyIC.h"
#include "TrampolineCompiler.h"
#include "jscntxtinlines.h"
#include "jscompartment.h"
#include "jsscope.h"

#include "jsgcinlines.h"
#include "jsinterpinlines.h"

using namespace js;
using namespace js::mjit;


js::mjit::CompilerAllocPolicy::CompilerAllocPolicy(JSContext *cx, Compiler &compiler)
: ContextAllocPolicy(cx),
  oomFlag(&compiler.oomInVector)
{
}
void
StackFrame::methodjitStaticAsserts()
{
        
#if defined(JS_CPU_X86)
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_)     == 0x18);
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) + 4 == 0x1C);
        JS_STATIC_ASSERT(offsetof(StackFrame, ncode_)    == 0x14);
        
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_)     == 24);
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) + 4 == 28);
        JS_STATIC_ASSERT(offsetof(StackFrame, ncode_)    == 20);
#elif defined(JS_CPU_X64)
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_)     == 0x30);
        JS_STATIC_ASSERT(offsetof(StackFrame, ncode_)    == 0x28);
#endif
}






























#ifdef JS_METHODJIT_PROFILE_STUBS
static const size_t STUB_CALLS_FOR_OP_COUNT = 255;
static uint32 StubCallsForOp[STUB_CALLS_FOR_OP_COUNT];
#endif

extern "C" void JaegerTrampolineReturn();

extern "C" void JS_FASTCALL
PushActiveVMFrame(VMFrame &f)
{
    f.entryfp->script()->compartment->jaegerCompartment->pushActiveFrame(&f);
    f.regs.fp()->setNativeReturnAddress(JS_FUNC_TO_DATA_PTR(void*, JaegerTrampolineReturn));
}

extern "C" void JS_FASTCALL
PopActiveVMFrame(VMFrame &f)
{
    f.entryfp->script()->compartment->jaegerCompartment->popActiveFrame();
}

extern "C" void JS_FASTCALL
SetVMFrameRegs(VMFrame &f)
{
    
    f.cx->stack.repointRegs(&f.regs);
}

#if defined(__APPLE__) || (defined(XP_WIN) && !defined(JS_CPU_X64)) || defined(XP_OS2)
# define SYMBOL_STRING(name) "_" #name
#else
# define SYMBOL_STRING(name) #name
#endif

JS_STATIC_ASSERT(offsetof(FrameRegs, sp) == 0);

#if defined(__linux__) && defined(JS_CPU_X64)
# define SYMBOL_STRING_RELOC(name) #name "@plt"
#else
# define SYMBOL_STRING_RELOC(name) SYMBOL_STRING(name)
#endif

#if (defined(XP_WIN) || defined(XP_OS2)) && defined(JS_CPU_X86)
# define SYMBOL_STRING_VMFRAME(name) "@" #name "@4"
#else
# define SYMBOL_STRING_VMFRAME(name) SYMBOL_STRING_RELOC(name)
#endif

#if defined(XP_MACOSX)
# define HIDE_SYMBOL(name) ".private_extern _" #name
#elif defined(__linux__)
# define HIDE_SYMBOL(name) ".hidden" #name
#else
# define HIDE_SYMBOL(name)
#endif

#if defined(__GNUC__) && !defined(_WIN64)


#ifdef JS_CPU_ARM
JS_STATIC_ASSERT(sizeof(VMFrame) % 8 == 0);
#else
JS_STATIC_ASSERT(sizeof(VMFrame) % 16 == 0);
#endif

# if defined(JS_CPU_X64)






JS_STATIC_ASSERT(offsetof(VMFrame, savedRBX) == 0x58);
JS_STATIC_ASSERT(VMFrame::offsetOfFp == 0x38);

JS_STATIC_ASSERT(JSVAL_TAG_MASK == 0xFFFF800000000000LL);
JS_STATIC_ASSERT(JSVAL_PAYLOAD_MASK == 0x00007FFFFFFFFFFFLL);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampoline) "\n"
SYMBOL_STRING(JaegerTrampoline) ":"       "\n"
    
    "pushq %rbp"                         "\n"
    "movq %rsp, %rbp"                    "\n"
    
    "pushq %r12"                         "\n"
    "pushq %r13"                         "\n"
    "pushq %r14"                         "\n"
    "pushq %r15"                         "\n"
    "pushq %rbx"                         "\n"

    
    "movq $0xFFFF800000000000, %r13"     "\n"
    "movq $0x00007FFFFFFFFFFF, %r14"     "\n"

    





    "pushq %rsi"                         "\n" 
    "pushq %rcx"                         "\n" 
    "pushq %rdi"                         "\n" 
    "pushq %rsi"                         "\n" 
    "movq  %rsi, %rbx"                   "\n"

    
    "subq  $0x28, %rsp"                  "\n"

    
    "pushq %r8"                          "\n"

    
    "pushq %rdx"                         "\n"
    "movq  %rsp, %rdi"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(SetVMFrameRegs) "\n"
    "movq  %rsp, %rdi"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(PushActiveVMFrame) "\n"

    
    "jmp *0(%rsp)"                      "\n"
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampolineReturn) "\n"
SYMBOL_STRING(JaegerTrampolineReturn) ":"       "\n"
    "or   %rdx, %rcx"                    "\n"
    "movq %rcx, 0x30(%rbx)"              "\n"
    "movq %rsp, %rdi"                    "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"

    "addq $0x58, %rsp"                   "\n"
    "popq %rbx"                          "\n"
    "popq %r15"                          "\n"
    "popq %r14"                          "\n"
    "popq %r13"                          "\n"
    "popq %r12"                          "\n"
    "popq %rbp"                          "\n"
    "movq $1, %rax"                      "\n"
    "ret"                                "\n"
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerThrowpoline)  "\n"
SYMBOL_STRING(JaegerThrowpoline) ":"        "\n"
    "movq %rsp, %rdi"                       "\n"
    "call " SYMBOL_STRING_RELOC(js_InternalThrow) "\n"
    "testq %rax, %rax"                      "\n"
    "je   throwpoline_exit"                 "\n"
    "jmp  *%rax"                            "\n"
  "throwpoline_exit:"                       "\n"
    "movq %rsp, %rdi"                       "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
    "addq $0x58, %rsp"                      "\n"
    "popq %rbx"                             "\n"
    "popq %r15"                             "\n"
    "popq %r14"                             "\n"
    "popq %r13"                             "\n"
    "popq %r12"                             "\n"
    "popq %rbp"                             "\n"
    "xorq %rax,%rax"                        "\n"
    "ret"                                   "\n"
);

# elif defined(JS_CPU_X86)








JS_STATIC_ASSERT(offsetof(VMFrame, savedEBX) == 0x2c);
JS_STATIC_ASSERT((VMFrame::offsetOfFp) == 0x1C);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampoline) "\n"
SYMBOL_STRING(JaegerTrampoline) ":"       "\n"
    
    "pushl %ebp"                         "\n"
    "movl %esp, %ebp"                    "\n"
    
    "pushl %esi"                         "\n"
    "pushl %edi"                         "\n"
    "pushl %ebx"                         "\n"

    

    "movl  12(%ebp), %ebx"               "\n"   
    "pushl %ebx"                         "\n"   
    "pushl 20(%ebp)"                     "\n"   
    "pushl 8(%ebp)"                      "\n"   
    "pushl %ebx"                         "\n"   
    "subl $0x1C, %esp"                   "\n"

    
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(SetVMFrameRegs) "\n"
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(PushActiveVMFrame) "\n"

    "jmp *16(%ebp)"                      "\n"
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampolineReturn) "\n"
SYMBOL_STRING(JaegerTrampolineReturn) ":" "\n"
    "movl  %edx, 0x18(%ebx)"             "\n"
    "movl  %ecx, 0x1C(%ebx)"             "\n"
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"

    "addl $0x2C, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    "movl $1, %eax"                      "\n"
    "ret"                                "\n"
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerThrowpoline)  "\n"
SYMBOL_STRING(JaegerThrowpoline) ":"        "\n"
    
    "pushl %esp"                         "\n"
    "pushl (%esp)"                       "\n"
    "pushl (%esp)"                       "\n"
    "pushl (%esp)"                       "\n"
    "call " SYMBOL_STRING_RELOC(js_InternalThrow) "\n"
    


    "addl $0x10, %esp"                   "\n"
    "testl %eax, %eax"                   "\n"
    "je   throwpoline_exit"              "\n"
    "jmp  *%eax"                         "\n"
  "throwpoline_exit:"                    "\n"
    "movl %esp, %ecx"                    "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
    "addl $0x2c, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    "xorl %eax, %eax"                    "\n"
    "ret"                                "\n"
);

# elif defined(JS_CPU_ARM)

JS_STATIC_ASSERT(sizeof(VMFrame) == 80);
JS_STATIC_ASSERT(offsetof(VMFrame, savedLR) ==          (4*19));
JS_STATIC_ASSERT(offsetof(VMFrame, entryfp) ==          (4*10));
JS_STATIC_ASSERT(offsetof(VMFrame, stackLimit) ==       (4*9));
JS_STATIC_ASSERT(offsetof(VMFrame, cx) ==               (4*8));
JS_STATIC_ASSERT(VMFrame::offsetOfFp ==                 (4*7));
JS_STATIC_ASSERT(offsetof(VMFrame, unused) ==           (4*4));
JS_STATIC_ASSERT(offsetof(VMFrame, previous) ==         (4*3));

JS_STATIC_ASSERT(JSFrameReg == JSC::ARMRegisters::r11);
JS_STATIC_ASSERT(JSReturnReg_Data == JSC::ARMRegisters::r1);
JS_STATIC_ASSERT(JSReturnReg_Type == JSC::ARMRegisters::r2);

#ifdef MOZ_THUMB2
#define FUNCTION_HEADER_EXTRA \
  ".align 2\n" \
  ".thumb\n" \
  ".thumb_func\n"
#else
#define FUNCTION_HEADER_EXTRA
#endif

asm (
".text\n"
FUNCTION_HEADER_EXTRA
".globl " SYMBOL_STRING(JaegerTrampoline)   "\n"
SYMBOL_STRING(JaegerTrampoline) ":"         "\n"
    




























    
    
"   push    {r4-r11,lr}"                        "\n"
    
"   push    {r1}"                               "\n"    
"   push    {r3}"                               "\n"    
"   push    {r0}"                               "\n"    
"   push    {r1}"                               "\n"    
    
"   sub     sp, sp, #(4*7)"                     "\n"

    
"   mov     r4, r2"                             "\n"
    
"   mov     r11, r1"                            "\n"

"   mov     r0, sp"                             "\n"
"   blx  " SYMBOL_STRING_VMFRAME(SetVMFrameRegs)   "\n"
"   mov     r0, sp"                             "\n"
"   blx  " SYMBOL_STRING_VMFRAME(PushActiveVMFrame)"\n"

    
"   bx     r4"                                  "\n"
);

asm (
".text\n"
FUNCTION_HEADER_EXTRA
".globl " SYMBOL_STRING(JaegerTrampolineReturn)   "\n"
SYMBOL_STRING(JaegerTrampolineReturn) ":"         "\n"
"   str r1, [r11, #24]"                    "\n" 
"   str r2, [r11, #28]"                    "\n" 

    
"   mov     r0, sp"                             "\n"
"   blx  " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"

    
"   add     sp, sp, #(4*7 + 4*4)"               "\n"

    
"   mov     r0, #1"                         "\n"
"   pop     {r4-r11,pc}"                    "\n"
);

asm (
".text\n"
FUNCTION_HEADER_EXTRA
".globl " SYMBOL_STRING(JaegerThrowpoline)  "\n"
SYMBOL_STRING(JaegerThrowpoline) ":"        "\n"
    
"   mov     r0, sp"                         "\n"

    
"   blx  " SYMBOL_STRING_RELOC(js_InternalThrow) "\n"
    
    

"   cmp     r0, #0"                         "\n"
"   it      ne"                             "\n"
"   bxne    r0"                             "\n"

    
"   mov     r0, sp"                             "\n"
"   blx  " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
"   add     sp, sp, #(4*7 + 4*4)"               "\n"
"   mov     r0, #0"                         "\n"
"   pop     {r4-r11,pc}"                    "\n"
);

asm (
".text\n"
FUNCTION_HEADER_EXTRA
".globl " SYMBOL_STRING(JaegerStubVeneer)   "\n"
SYMBOL_STRING(JaegerStubVeneer) ":"         "\n"
    




"   push    {ip,lr}"                        "\n"
"   blx     ip"                             "\n"
"   pop     {ip,pc}"                        "\n"
);

# elif defined(JS_CPU_SPARC)
# else
#  error "Unsupported CPU!"
# endif
#elif defined(_MSC_VER) && defined(JS_CPU_X86)








JS_STATIC_ASSERT(offsetof(VMFrame, savedEBX) == 0x2c);
JS_STATIC_ASSERT(VMFrame::offsetOfFp == 0x1C);

extern "C" {

    __declspec(naked) JSBool JaegerTrampoline(JSContext *cx, StackFrame *fp, void *code,
                                              Value *stackLimit)
    {
        __asm {
            
            push ebp;
            mov ebp, esp;
            
            push esi;
            push edi;
            push ebx;

            

            mov  ebx, [ebp + 12];
            push ebx;
            push [ebp + 20];
            push [ebp + 8];
            push ebx;
            sub  esp, 0x1C;

            
            mov  ecx, esp;
            call SetVMFrameRegs;
            mov  ecx, esp;
            call PushActiveVMFrame;

            jmp dword ptr [ebp + 16];
        }
    }

    __declspec(naked) void JaegerTrampolineReturn()
    {
        __asm {
            mov [ebx + 0x18], edx;
            mov [ebx + 0x1C], ecx;
            mov  ecx, esp;
            call PopActiveVMFrame;

            add esp, 0x2C;

            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            mov eax, 1;
            ret;
        }
    }

    extern "C" void *js_InternalThrow(js::VMFrame &f);

    __declspec(naked) void *JaegerThrowpoline(js::VMFrame *vmFrame) {
        __asm {
            
            push esp;
            push [esp];
            push [esp];
            push [esp];
            call js_InternalThrow;
            


            add esp, 0x10;
            test eax, eax;
            je throwpoline_exit;
            jmp eax;
        throwpoline_exit:
            mov ecx, esp;
            call PopActiveVMFrame;
            add esp, 0x2c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            xor eax, eax
            ret;
        }
    }
}



#elif defined(_WIN64)






JS_STATIC_ASSERT(offsetof(VMFrame, savedRBX) == 0x58);
JS_STATIC_ASSERT(VMFrame::offsetOfFp == 0x38);
JS_STATIC_ASSERT(JSVAL_TAG_MASK == 0xFFFF800000000000LL);
JS_STATIC_ASSERT(JSVAL_PAYLOAD_MASK == 0x00007FFFFFFFFFFFLL);

#endif                   

bool
JaegerCompartment::Initialize()
{
    execAlloc_ = js::OffTheBooks::new_<JSC::ExecutableAllocator>();
    if (!execAlloc_)
        return false;
    
    TrampolineCompiler tc(execAlloc_, &trampolines);
    if (!tc.compile()) {
        delete execAlloc_;
        return false;
    }

#ifdef JS_METHODJIT_PROFILE_STUBS
    for (size_t i = 0; i < STUB_CALLS_FOR_OP_COUNT; ++i)
        StubCallsForOp[i] = 0;
#endif

    activeFrame_ = NULL;

    return true;
}

void
JaegerCompartment::Finish()
{
    TrampolineCompiler::release(&trampolines);
    Foreground::delete_(execAlloc_);
#ifdef JS_METHODJIT_PROFILE_STUBS
    FILE *fp = fopen("/tmp/stub-profiling", "wt");
# define OPDEF(op,val,name,image,length,nuses,ndefs,prec,format) \
    fprintf(fp, "%03d %s %d\n", val, #op, StubCallsForOp[val]);
# include "jsopcode.tbl"
# undef OPDEF
    fclose(fp);
#endif
}

extern "C" JSBool
JaegerTrampoline(JSContext *cx, StackFrame *fp, void *code, Value *stackLimit);

JSBool
mjit::EnterMethodJIT(JSContext *cx, StackFrame *fp, void *code, Value *stackLimit)
{
#ifdef JS_METHODJIT_SPEW
    Profiler prof;
    JSScript *script = fp->script();

    JaegerSpew(JSpew_Prof, "%s jaeger script, line %d\n",
               script->filename, script->lineno);
    prof.start();
#endif

    JS_ASSERT(cx->fp() == fp);
    FrameRegs &oldRegs = cx->regs();

    JSBool ok;
    {
        AssertCompartmentUnchanged pcc(cx);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);
        ok = JaegerTrampoline(cx, fp, code, stackLimit);
    }

    
    cx->stack.repointRegs(&oldRegs);
    JS_ASSERT(fp == cx->fp());

    
    fp->markReturnValue();

    
    fp->markActivationObjectsAsPut();

#ifdef JS_METHODJIT_SPEW
    prof.stop();
    JaegerSpew(JSpew_Prof, "script run took %d ms\n", prof.time_ms());
#endif

    return ok;
}

static inline JSBool
CheckStackAndEnterMethodJIT(JSContext *cx, StackFrame *fp, void *code)
{
    JS_CHECK_RECURSION(cx, return false);

    Value *stackLimit = cx->stack.space().getStackLimit(cx);
    if (!stackLimit)
        return false;

    return EnterMethodJIT(cx, fp, code, stackLimit);
}

JSBool
mjit::JaegerShot(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    JSScript *script = fp->script();
    JITScript *jit = script->getJIT(fp->isConstructing());

#ifdef JS_TRACER
    if (TRACE_RECORDER(cx))
        AbortRecording(cx, "attempt to enter method JIT while recording");
#endif

    JS_ASSERT(cx->regs().pc == script->code);

    return CheckStackAndEnterMethodJIT(cx, cx->fp(), jit->invokeEntry);
}

JSBool
js::mjit::JaegerShotAtSafePoint(JSContext *cx, void *safePoint)
{
#ifdef JS_TRACER
    JS_ASSERT(!TRACE_RECORDER(cx));
#endif

    return CheckStackAndEnterMethodJIT(cx, cx->fp(), safePoint);
}

NativeMapEntry *
JITScript::nmap() const
{
    return (NativeMapEntry *)((char*)this + sizeof(JITScript));
}

char *
JITScript::nmapSectionLimit() const
{
    return (char *)nmap() + sizeof(NativeMapEntry) * nNmapPairs;
}

#ifdef JS_MONOIC
ic::GetGlobalNameIC *
JITScript::getGlobalNames() const
{
    return (ic::GetGlobalNameIC *)nmapSectionLimit();
}

ic::SetGlobalNameIC *
JITScript::setGlobalNames() const
{
    return (ic::SetGlobalNameIC *)((char *)nmapSectionLimit() +
            sizeof(ic::GetGlobalNameIC) * nGetGlobalNames);
}

ic::CallICInfo *
JITScript::callICs() const
{
    return (ic::CallICInfo *)((char *)setGlobalNames() +
            sizeof(ic::SetGlobalNameIC) * nSetGlobalNames);
}

ic::EqualityICInfo *
JITScript::equalityICs() const
{
    return (ic::EqualityICInfo *)((char *)callICs() + sizeof(ic::CallICInfo) * nCallICs);
}

ic::TraceICInfo *
JITScript::traceICs() const
{
    return (ic::TraceICInfo *)((char *)equalityICs() + sizeof(ic::EqualityICInfo) * nEqualityICs);
}

char *
JITScript::monoICSectionsLimit() const
{
    return (char *)traceICs() + sizeof(ic::TraceICInfo) * nTraceICs;
}
#else   
char *
JITScript::monoICSectionsLimit() const
{
    return nmapSectionsLimit();
}
#endif  

#ifdef JS_POLYIC
ic::GetElementIC *
JITScript::getElems() const
{
    return (ic::GetElementIC *)monoICSectionsLimit();
}

ic::SetElementIC *
JITScript::setElems() const
{
    return (ic::SetElementIC *)((char *)getElems() + sizeof(ic::GetElementIC) * nGetElems);
}

ic::PICInfo *
JITScript::pics() const
{
    return (ic::PICInfo *)((char *)setElems() + sizeof(ic::SetElementIC) * nSetElems);
}

char *
JITScript::polyICSectionsLimit() const
{
    return (char *)pics() + sizeof(ic::PICInfo) * nPICs;
}
#else   
char *
JITScript::polyICSectionsLimit() const
{
    return monoICSectionsLimit();
}
#endif  

js::mjit::CallSite *
JITScript::callSites() const
{
    return (js::mjit::CallSite *)polyICSectionsLimit();
}

template <typename T>
static inline void Destroy(T &t)
{
    t.~T();
}

mjit::JITScript::~JITScript()
{
#if defined DEBUG && (defined JS_CPU_X86 || defined JS_CPU_X64) 
    void *addr = code.m_code.executableAddress();
    memset(addr, 0xcc, code.m_size);
#endif

    code.m_executablePool->release();

#if defined JS_POLYIC
    ic::GetElementIC *getElems_ = getElems();
    ic::SetElementIC *setElems_ = setElems();
    ic::PICInfo *pics_ = pics();
    for (uint32 i = 0; i < nGetElems; i++)
        Destroy(getElems_[i]);
    for (uint32 i = 0; i < nSetElems; i++)
        Destroy(setElems_[i]);
    for (uint32 i = 0; i < nPICs; i++)
        Destroy(pics_[i]);
#endif

#if defined JS_MONOIC
    for (JSC::ExecutablePool **pExecPool = execPools.begin();
         pExecPool != execPools.end();
         ++pExecPool)
    {
        (*pExecPool)->release();
    }
    
    ic::CallICInfo *callICs_ = callICs();
    for (uint32 i = 0; i < nCallICs; i++)
        callICs_[i].releasePools();
#endif
}


size_t
mjit::JITScript::scriptDataSize()
{
    return sizeof(JITScript) +
        sizeof(NativeMapEntry) * nNmapPairs +
#if defined JS_MONOIC
        sizeof(ic::GetGlobalNameIC) * nGetGlobalNames +
        sizeof(ic::SetGlobalNameIC) * nSetGlobalNames +
        sizeof(ic::CallICInfo) * nCallICs +
        sizeof(ic::EqualityICInfo) * nEqualityICs +
        sizeof(ic::TraceICInfo) * nTraceICs +
#endif
#if defined JS_POLYIC
        sizeof(ic::PICInfo) * nPICs +
        sizeof(ic::GetElementIC) * nGetElems +
        sizeof(ic::SetElementIC) * nSetElems +
#endif
        sizeof(CallSite) * nCallSites;
}

void
mjit::ReleaseScriptCode(JSContext *cx, JSScript *script)
{
    
    
    
    JITScript *jscr;

    if ((jscr = script->jitNormal)) {
        cx->runtime->mjitDataSize -= jscr->scriptDataSize();
#ifdef DEBUG
        if (jscr->pcProfile) {
            cx->free_(jscr->pcProfile);
            jscr->pcProfile = NULL;
        }
#endif

        jscr->~JITScript();
        cx->free_(jscr);
        script->jitNormal = NULL;
        script->jitArityCheckNormal = NULL;
    }

    if ((jscr = script->jitCtor)) {
        cx->runtime->mjitDataSize -= jscr->scriptDataSize();
#ifdef DEBUG
        if (jscr->pcProfile) {
            cx->free_(jscr->pcProfile);
            jscr->pcProfile = NULL;
        }
#endif

        jscr->~JITScript();
        cx->free_(jscr);
        script->jitCtor = NULL;
        script->jitArityCheckCtor = NULL;
    }
}

#ifdef JS_METHODJIT_PROFILE_STUBS
void JS_FASTCALL
mjit::ProfileStubCall(VMFrame &f)
{
    JSOp op = JSOp(*f.regs.pc);
    StubCallsForOp[op]++;
}
#endif

#ifdef JS_POLYIC
static int
PICPCComparator(const void *key, const void *entry)
{
    const jsbytecode *pc = (const jsbytecode *)key;
    const ic::PICInfo *pic = (const ic::PICInfo *)entry;

    if (ic::PICInfo::CALL != pic->kind)
        return ic::PICInfo::CALL - pic->kind;

    





    if (pc < pic->pc)
        return -1;
    else if (pc == pic->pc)
        return 0;
    else
        return 1;
}

uintN
mjit::GetCallTargetCount(JSScript *script, jsbytecode *pc)
{
    ic::PICInfo *pic;
    
    if (mjit::JITScript *jit = script->getJIT(false)) {
        pic = (ic::PICInfo *)bsearch(pc, jit->pics(), jit->nPICs, sizeof(ic::PICInfo),
                                     PICPCComparator);
        if (pic)
            return pic->stubsGenerated + 1; 
    }
    
    if (mjit::JITScript *jit = script->getJIT(true)) {
        pic = (ic::PICInfo *)bsearch(pc, jit->pics(), jit->nPICs, sizeof(ic::PICInfo),
                                     PICPCComparator);
        if (pic)
            return pic->stubsGenerated + 1; 
    }

    return 1;
}
#else
uintN
mjit::GetCallTargetCount(JSScript *script, jsbytecode *pc)
{
    return 1;
}
#endif

jsbytecode *
JITScript::nativeToPC(void *returnAddress) const
{
    size_t low = 0;
    size_t high = nCallICs;
    js::mjit::ic::CallICInfo *callICs_ = callICs();
    while (high > low + 1) {
        
        size_t mid = (high + low) / 2;
        void *entry = callICs_[mid].funGuard.executableAddress();

        



        if (entry >= returnAddress)
            high = mid;
        else
            low = mid;
    }

    js::mjit::ic::CallICInfo &ic = callICs_[low];

    JS_ASSERT((uint8*)ic.funGuard.executableAddress() + ic.joinPointOffset == returnAddress);
    return ic.pc;
}

#ifdef JS_METHODJIT_SPEW
static void
DumpProfile(JSContext *cx, JSScript *script, JITScript* jit, bool isCtor)
{
    JS_ASSERT(!cx->runtime->gcRunning);

#ifdef DEBUG
    if (IsJaegerSpewChannelActive(JSpew_PCProf) && jit->pcProfile) {
        
        AutoArenaAllocator(&cx->tempPool);
        Sprinter sprinter;
        INIT_SPRINTER(cx, &sprinter, &cx->tempPool, 0);
        js_Disassemble(cx, script, true, &sprinter, jit->pcProfile);
        fprintf(stdout, "--- PC PROFILE %s:%d%s ---\n", script->filename, script->lineno,
                isCtor ? " (constructor)" : "");
        fprintf(stdout, "%s\n", sprinter.base);
        fprintf(stdout, "--- END PC PROFILE %s:%d%s ---\n", script->filename, script->lineno,
                isCtor ? " (constructor)" : "");
    }
#endif
}
#endif

void
mjit::DumpAllProfiles(JSContext *cx)
{
#ifdef JS_METHODJIT_SPEW
    for (JSScript *script = (JSScript *) JS_LIST_HEAD(&cx->compartment->scripts);
         script != (JSScript *) &cx->compartment->scripts;
         script = (JSScript *) JS_NEXT_LINK((JSCList *)script))
    {
        if (script->jitCtor)
            DumpProfile(cx, script, script->jitCtor, true);
        if (script->jitNormal)
            DumpProfile(cx, script, script->jitNormal, false);
    }
#endif
}
