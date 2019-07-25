





































#include "MethodJIT.h"
#include "Logging.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "jstracer.h"
#include "BaseAssembler.h"
#include "MonoIC.h"
#include "PolyIC.h"
#include "TrampolineCompiler.h"

using namespace js;
using namespace js::mjit;

#ifdef JS_METHODJIT_PROFILE_STUBS
static const size_t STUB_CALLS_FOR_OP_COUNT = 255;
static uint32 StubCallsForOp[STUB_CALLS_FOR_OP_COUNT];
#endif

extern "C" void JS_FASTCALL
PushActiveVMFrame(VMFrame &f)
{
    f.previous = JS_METHODJIT_DATA(f.cx).activeFrame;
    JS_METHODJIT_DATA(f.cx).activeFrame = &f;
}

extern "C" void JS_FASTCALL
PopActiveVMFrame(VMFrame &f)
{
    JS_ASSERT(JS_METHODJIT_DATA(f.cx).activeFrame);
    JS_METHODJIT_DATA(f.cx).activeFrame = JS_METHODJIT_DATA(f.cx).activeFrame->previous;    
}

extern "C" void JS_FASTCALL
SetVMFrameRegs(VMFrame &f)
{
    f.oldRegs = f.cx->regs;
    f.cx->setCurrentRegs(&f.regs);
}

extern "C" void JS_FASTCALL
UnsetVMFrameRegs(VMFrame &f)
{
    *f.oldRegs = f.regs;
    f.cx->setCurrentRegs(f.oldRegs);
}

#if defined(__APPLE__) || defined(XP_WIN)
# define SYMBOL_STRING(name) "_" #name
#else
# define SYMBOL_STRING(name) #name
#endif

JS_STATIC_ASSERT(offsetof(JSFrameRegs, sp) == 0);

#if defined(__linux__) && defined(JS_CPU_X64)
# define SYMBOL_STRING_RELOC(name) #name "@plt"
#else
# define SYMBOL_STRING_RELOC(name) SYMBOL_STRING(name)
#endif

#if defined(XP_MACOSX)
# define HIDE_SYMBOL(name) ".private_extern _" #name
#elif defined(__linux__)
# define HIDE_SYMBOL(name) ".hidden" #name
#else
# define HIDE_SYMBOL(name)
#endif

#if defined(__GNUC__)


#ifdef JS_CPU_ARM
JS_STATIC_ASSERT(sizeof(VMFrame) % 8 == 0);
#else
JS_STATIC_ASSERT(sizeof(VMFrame) % 16 == 0);
#endif

# if defined(JS_CPU_X64)






JS_STATIC_ASSERT(offsetof(VMFrame, savedRBX) == 0x58);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 0x40);

asm volatile (
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

    





    "pushq %rcx"                         "\n"
    "pushq %rdi"                         "\n"
    "pushq %rsi"                         "\n"
    "movq  %rsi, %rbx"                   "\n"

    
    "subq  $0x38, %rsp"                  "\n"

    
    "pushq %rdx"                         "\n"
    "movq  %rsp, %rdi"                   "\n"
    "call " SYMBOL_STRING_RELOC(SetVMFrameRegs) "\n"
    "movq  %rsp, %rdi"                   "\n"
    "call " SYMBOL_STRING_RELOC(PushActiveVMFrame) "\n"
    "popq  %rdx"                         "\n"

    



    "call *%rdx"                         "\n"
    "leaq -8(%rsp), %rdi"                "\n"
    "call " SYMBOL_STRING_RELOC(PopActiveVMFrame) "\n"
    "leaq -8(%rsp), %rdi"                "\n"
    "call " SYMBOL_STRING_RELOC(UnsetVMFrameRegs) "\n"

    "addq $0x50, %rsp"                   "\n"
    "popq %rbx"                          "\n"
    "popq %r15"                          "\n"
    "popq %r14"                          "\n"
    "popq %r13"                          "\n"
    "popq %r12"                          "\n"
    "popq %rbp"                          "\n"
    "movq $1, %rax"                      "\n"
    "ret"                                "\n"
);

asm volatile (
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
    "call " SYMBOL_STRING_RELOC(PopActiveVMFrame) "\n"
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

JS_STATIC_ASSERT(offsetof(JSStackFrame, rval) == 0x40);
JS_STATIC_ASSERT(offsetof(JSStackFrame, ncode) == 0x60);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 0x40);

JS_STATIC_ASSERT(JSVAL_TAG_MASK == 0xFFFF800000000000LL);
JS_STATIC_ASSERT(JSVAL_PAYLOAD_MASK == 0x00007FFFFFFFFFFFLL);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerFromTracer)   "\n"
SYMBOL_STRING(JaegerFromTracer) ":"         "\n"
    "movq 0x40(%rbx), %rcx"                 "\n" 
    "movq $0xFFFF800000000000, %r11"         "\n" 
    "andq %r11, %rcx"                       "\n" 

    "movq 0x40(%rbx), %rdx"                 "\n" 
    "movq $0x00007FFFFFFFFFFF, %r11"        "\n" 
    "andq %r11, %rdx"                       "\n" 

    "movq 0x60(%rbx), %rax"                 "\n" 
    "movq 0x40(%rsp), %rbx"                 "\n" 
    "ret"                                   "\n"
);

# elif defined(JS_CPU_X86)








JS_STATIC_ASSERT(offsetof(VMFrame, savedEBX) == 0x2c);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 0x20);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampoline) "\n"
SYMBOL_STRING(JaegerTrampoline) ":"       "\n"
    
    "pushl %ebp"                         "\n"
    "movl %esp, %ebp"                    "\n"
    
    "pushl %esi"                         "\n"
    "pushl %edi"                         "\n"
    "pushl %ebx"                         "\n"

    

    "pushl 20(%ebp)"                     "\n"
    "pushl 8(%ebp)"                      "\n"
    "pushl 12(%ebp)"                     "\n"
    "movl  12(%ebp), %ebx"               "\n"
    "subl $0x1c, %esp"                   "\n"

    
    "pushl 16(%ebp)"                     "\n"
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_RELOC(SetVMFrameRegs) "\n"
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_RELOC(PushActiveVMFrame) "\n"
    "popl  %edx"                         "\n"

    "call  *%edx"                        "\n"
    "leal  -4(%esp), %ecx"               "\n"
    "call " SYMBOL_STRING_RELOC(PopActiveVMFrame) "\n"
    "leal  -4(%esp), %ecx"               "\n"
    "call " SYMBOL_STRING_RELOC(UnsetVMFrameRegs) "\n"

    "addl $0x28, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    "movl $1, %eax"                      "\n"
    "ret"                                "\n"
);

asm volatile (
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
    "call " SYMBOL_STRING_RELOC(PopActiveVMFrame) "\n"
    "addl $0x2c, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    "xorl %eax, %eax"                    "\n"
    "ret"                                "\n"
);

JS_STATIC_ASSERT(offsetof(JSStackFrame, rval) == 0x28);
JS_STATIC_ASSERT(offsetof(JSStackFrame, ncode) == 0x3C);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 0x20);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerFromTracer)   "\n"
SYMBOL_STRING(JaegerFromTracer) ":"         "\n"
    "movl 0x28(%ebx), %edx"                 "\n" 
    "movl 0x2C(%ebx), %ecx"                 "\n" 
    "movl 0x3C(%ebx), %eax"                 "\n" 
    "movl 0x20(%esp), %ebx"                 "\n" 
    "ret"                                   "\n"
);

# elif defined(JS_CPU_ARM)

JS_STATIC_ASSERT(offsetof(VMFrame, savedLR) == (sizeof(VMFrame)-4));
JS_STATIC_ASSERT(sizeof(VMFrame) == 80);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerFromTracer)   "\n"
SYMBOL_STRING(JaegerFromTracer) ":"         "\n"
    
    "ldr r11, [sp, #32]"                    "\n"
    "bx  r0"                                "\n"
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampoline)   "\n"
SYMBOL_STRING(JaegerTrampoline) ":"         "\n"
    



























    
    


"   push    {r4-r11,lr}"                        "\n"
    
"   push    {r0,r3}"                            "\n"    
"   push    {r1}"                               "\n"    
    
"   sub     sp, sp, #(4*8)"                     "\n"

"   mov     r0, sp"                             "\n"
"   mov     r4, r2"                             "\n"    
"   bl  " SYMBOL_STRING_RELOC(SetVMFrameRegs)   "\n"
"   mov     r0, sp"                             "\n"
"   bl  " SYMBOL_STRING_RELOC(PushActiveVMFrame)"\n"

    

"   add     sp, sp, #(4*1)"                     "\n"
"   blx     r4"                                 "\n"
"   sub     sp, sp, #(4*1)"                     "\n"

    
"   mov     r0, sp"                             "\n"
"   bl  " SYMBOL_STRING_RELOC(PopActiveVMFrame) "\n"
"   mov     r0, sp"                             "\n"
"   bl  " SYMBOL_STRING_RELOC(UnsetVMFrameRegs) "\n"

    
"   add     sp, sp, #(4*8 + 4*3)"               "\n"

    
"   mov     r0, #1"                         "\n"
"   pop     {r4-r11,pc}"                    "\n"
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerThrowpoline)  "\n"
SYMBOL_STRING(JaegerThrowpoline) ":"        "\n"
    
"   mov     r0, sp"                         "\n"

    
"   bl  " SYMBOL_STRING_RELOC(js_InternalThrow) "\n"
    
    

"   cmp     r0, #0"                         "\n"
"   bxne    r0"                             "\n"

    
"   add     sp, sp, #(4*8 + 4*3)"               "\n"

"   pop     {r4-r11,pc}"                    "\n"
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerStubVeneer)   "\n"
SYMBOL_STRING(JaegerStubVeneer) ":"         "\n"
    




"   push    {ip,lr}"                        "\n"
"   blx     ip"                             "\n"
"   pop     {ip,pc}"                        "\n"
);

# else
#  error "Unsupported CPU!"
# endif
#elif defined(_MSC_VER)

#if defined(JS_CPU_X86)








JS_STATIC_ASSERT(offsetof(VMFrame, savedEBX) == 0x2c);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 0x20);

extern "C" {

    __declspec(naked) void JaegerFromTracer()
    {
        __asm {
            mov edx, [ebx + 0x28];
            mov ecx, [ebx + 0x2C];
            mov eax, [ebx + 0x3C];
            mov ebx, [esp + 0x20];
            ret;
        }
    }

    __declspec(naked) JSBool JaegerTrampoline(JSContext *cx, JSStackFrame *fp, void *code,
                                              uintptr_t inlineCallCount)
    {
        __asm {
            
            push ebp;
            mov ebp, esp;
            
            push esi;
            push edi;
            push ebx;

            

            push [ebp+20];
            push [ebp+8];
            push [ebp+12];
            mov  ebx, [ebp+12];
            sub  esp, 0x1c;

            
            push [ebp+16];
            mov  ecx, esp;
            call SetVMFrameRegs;
            mov  ecx, esp;
            call PushActiveVMFrame;
            pop  edx;

            call edx;
            lea  ecx, [esp-4];
            call PopActiveVMFrame;
            lea  ecx, [esp-4];
            call UnsetVMFrameRegs;

            add esp, 0x28

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

#elif defined(JS_CPU_X64)






JS_STATIC_ASSERT(offsetof(VMFrame, savedRBX) == 0x48);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 0x30);



#else
#  error "Unsupported CPU!"
#endif

#endif                   

bool
ThreadData::Initialize()
{
    execPool = new JSC::ExecutableAllocator();
    if (!execPool)
        return false;
    
    TrampolineCompiler tc(execPool, &trampolines);
    if (!tc.compile()) {
        delete execPool;
        return false;
    }

#ifdef JS_METHODJIT_PROFILE_STUBS
    for (size_t i = 0; i < STUB_CALLS_FOR_OP_COUNT; ++i)
        StubCallsForOp[i] = 0;
#endif

    activeFrame = NULL;

    return true;
}

void
ThreadData::Finish()
{
    TrampolineCompiler::release(&trampolines);
    delete execPool;
#ifdef JS_METHODJIT_PROFILE_STUBS
    FILE *fp = fopen("/tmp/stub-profiling", "wt");
# define OPDEF(op,val,name,image,length,nuses,ndefs,prec,format) \
    fprintf(fp, "%03d %s %d\n", val, #op, StubCallsForOp[val]);
# include "jsopcode.tbl"
# undef OPDEF
    fclose(fp);
#endif
}

extern "C" JSBool JaegerTrampoline(JSContext *cx, JSStackFrame *fp, void *code,
                                   uintptr_t inlineCallCount);

JSBool
mjit::JaegerShot(JSContext *cx)
{
    JS_ASSERT(cx->regs);

    JS_CHECK_RECURSION(cx, return JS_FALSE;);

    void *code;
    jsbytecode *pc = cx->regs->pc;
    JSStackFrame *fp = cx->fp;
    JSScript *script = fp->script;
    uintptr_t inlineCallCount = 0;

    JS_ASSERT(script->ncode && script->ncode != JS_UNJITTABLE_METHOD);

#ifdef JS_TRACER
    if (TRACE_RECORDER(cx))
        AbortRecording(cx, "attempt to enter method JIT while recording");
#endif

    if (pc == script->code)
        code = script->nmap[-1];
    else
        code = script->nmap[pc - script->code];

    JS_ASSERT(code);

#ifdef JS_METHODJIT_SPEW
    Profiler prof;

    JaegerSpew(JSpew_Prof, "entering jaeger script: %s, line %d\n", fp->script->filename,
               fp->script->lineno);
    prof.start();
#endif

#ifdef DEBUG
    JSStackFrame *checkFp = fp;
#endif
#if 0
    uintptr_t iCC = inlineCallCount;
    while (iCC--)
        checkFp = checkFp->down;
#endif

    JSAutoResolveFlags rf(cx, JSRESOLVE_INFER);
    JSBool ok = JaegerTrampoline(cx, fp, code, inlineCallCount);

    JS_ASSERT(checkFp == cx->fp);

#ifdef JS_METHODJIT_SPEW
    prof.stop();
    JaegerSpew(JSpew_Prof, "script run took %d ms\n", prof.time_ms());
#endif

    return ok;
}

template <typename T>
static inline void Destroy(T &t)
{
    t.~T();
}

void
mjit::ReleaseScriptCode(JSContext *cx, JSScript *script)
{
    if (script->execPool) {
#if defined DEBUG && (defined JS_CPU_X86 || defined JS_CPU_X64) 
        memset(script->nmap[-1], 0xcc, script->inlineLength + script->outOfLineLength);
#endif
        script->execPool->release();
        script->execPool = NULL;
        
        script->ncode = NULL;
        script->inlineLength = 0;
        script->outOfLineLength = 0;

#if defined JS_POLYIC
        if (script->pics) {
            uint32 npics = script->numPICs();
            for (uint32 i = 0; i < npics; i++) {
                script->pics[i].releasePools();
                Destroy(script->pics[i].execPools);
            }
            cx->free((uint8*)script->pics - sizeof(uint32));
            script->pics = NULL;
        }
#endif
    }

    if (script->nmap) {
        cx->free(script->nmap - 1);
        script->nmap = NULL;
    }
    if (script->callSites) {
        cx->free(script->callSites - 1);
        script->callSites = NULL;
    }
#if defined JS_MONOIC
    if (script->mics) {
        cx->free((uint8*)script->mics - sizeof(uint32));
        script->mics = NULL;
    }
#endif

# if 0 
    if (script->trees) {
        cx->free(script->trees);
        script->trees = NULL;
    }
# endif
}

#ifdef JS_METHODJIT_PROFILE_STUBS
void JS_FASTCALL
mjit::ProfileStubCall(VMFrame &f)
{
    JSOp op = JSOp(*f.regs.pc);
    StubCallsForOp[op]++;
}
#endif

