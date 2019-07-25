





































#include "MethodJIT.h"
#include "Logging.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "jstracer.h"

using namespace js;
using namespace js::mjit;

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






JS_STATIC_ASSERT(offsetof(VMFrame, savedRBX) == 0x48);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 0x30);

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

    
    "subq  $0x28, %rsp"                  "\n"

    
    "pushq %rdx"                         "\n"
    "movq  %rsp, %rdi"                   "\n"
    "call " SYMBOL_STRING_RELOC(SetVMFrameRegs) "\n"
    "popq  %rdx"                         "\n"

    



    "call *%rdx"                         "\n"
    "leaq -8(%rsp), %rdi"                "\n"
    "call " SYMBOL_STRING_RELOC(UnsetVMFrameRegs) "\n"

    "addq $0x40, %rsp"                   "\n"
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
    "addq $0x48, %rsp"                      "\n"
    "popq %rbx"                             "\n"
    "popq %r15"                             "\n"
    "popq %r14"                             "\n"
    "popq %r13"                             "\n"
    "popq %r12"                             "\n"
    "popq %rbp"                             "\n"
    "ret"                                   "\n"
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerFromTracer)   "\n"
SYMBOL_STRING(JaegerFromTracer) ":"         "\n"
    
    "movq 0x30(%rsp), %rbx"                 "\n"
    "jmp *%rax"                             "\n"
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
    "popl  %edx"                         "\n"

    "call  *%edx"                        "\n"
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
    "addl $0x2c, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    "xorl %eax, %eax"                    "\n"
    "ret"                                "\n"
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerFromTracer)   "\n"
SYMBOL_STRING(JaegerFromTracer) ":"         "\n"
    
    "movl 0x20(%esp), %ebx"                 "\n"
    "jmp *%eax"                             "\n"
);

# elif defined(JS_CPU_ARM)

JS_STATIC_ASSERT(offsetof(VMFrame, savedLR) == 76);
JS_STATIC_ASSERT(offsetof(VMFrame, fp) == 32);

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
    



















    
    

"   push    {r10,r11,lr}"                   "\n"
"   push    {r7-r9}"                        "\n"
"   push    {r4-r6}"                        "\n"
"   mov     r11, #0"                        "\n"   
"   push    {r11}"                          "\n"   
"   push    {r0}"                           "\n"   
"   push    {r1}"                           "\n"   
"   mov     r11, r1"                        "\n"   

    



"   sub     sp, sp, #(8*4)"                 "\n"

"   mov     r0, sp"                         "\n"
"   push    {r2}"                           "\n"
"   blx " SYMBOL_STRING_RELOC(SetVMFrameRegs) "\n"
"   pop     {r2}"                           "\n"

    
"   bl  " SYMBOL_STRING_RELOC(JaegerTrampVeneer) "\n"

    



"   mov     r0, sp"                         "\n"
"   blx " SYMBOL_STRING_RELOC(UnsetVMFrameRegs) "\n"

    
"   add     sp, sp, #(11*4)"           "\n"

    
"   mov     r0, #1"                         "\n"

    

"   pop     {r4-r6}"                        "\n"
"   pop     {r7-r9}"                        "\n"
"   pop     {r10,r11,pc}"                   "\n"    
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerThrowpoline)  "\n"
SYMBOL_STRING(JaegerThrowpoline) ":"        "\n"
    
"   mov     r0, sp"                         "\n"

    
"   blx " SYMBOL_STRING_RELOC(js_InternalThrow) "\n"
    
    

"   cmp     r0, #0"                         "\n"
"   bxne    r0"                             "\n"

    
"   add     sp, sp, #(11*4)"           "\n"

    

"   pop     {r4-r6}"                        "\n"
"   pop     {r7-r9}"                        "\n"
"   pop     {r10,r11,pc}"                   "\n"    
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerStubVeneer)   "\n"
SYMBOL_STRING(JaegerStubVeneer) ":"         "\n"
    





"   str     lr, [sp]"                       "\n"    
"   blx     ip"                             "\n"
"   ldr     pc, [sp]"                       "\n"    
);

asm volatile (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampVeneer)   "\n"
SYMBOL_STRING(JaegerTrampVeneer) ":"         "\n"
    


"   str     lr, [sp, #4]"                   "\n"    
"   bx      r2"                             "\n"
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
            mov ebx, [esp + 0x20];
            jmp eax;
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
            pop  edx;

            call edx;
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
    
    if (!picScripts.init()) {
        delete execPool;
        return false;
    }

    return true;
}

void
ThreadData::Finish()
{
    delete execPool;
}

bool
ThreadData::addScript(JSScript *script)
{
    ScriptSet::AddPtr p = picScripts.lookupForAdd(script);
    if (p)
        return true;
    return picScripts.add(p, script);
}

void
ThreadData::removeScript(JSScript *script)
{
    ScriptSet::Ptr p = picScripts.lookup(script);
    if (p)
        picScripts.remove(p);
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
        code = script->ncode;
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

void
mjit::ReleaseScriptCode(JSContext *cx, JSScript *script)
{
    if (script->execPool) {
        script->execPool->release();
        script->execPool = NULL;
        
        script->ncode = NULL;
#ifdef DEBUG
        script->jitLength = 0;
#endif
        script->npics = 0;
        
#if defined(ENABLE_PIC) && ENABLE_PIC
        if (script->pics) {
            delete[] script->pics;
            script->pics = NULL;
            JS_METHODJIT_DATA(cx).removeScript(script);
        }
#endif
    }

    if (script->nmap) {
        cx->free(script->nmap);
        script->nmap = NULL;
    }
# if 0 
    if (script->trees) {
        cx->free(script->trees);
        script->trees = NULL;
    }
# endif
}

