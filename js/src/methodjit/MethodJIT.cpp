





































#include "MethodJIT.h"
#include "Logging.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "jstracer.h"
#include "jsgcmark.h"
#include "BaseAssembler.h"
#include "Compiler.h"
#include "MonoIC.h"
#include "PolyIC.h"
#include "TrampolineCompiler.h"
#include "jscntxtinlines.h"
#include "jscompartment.h"
#include "jsscope.h"
#include "jsgcmark.h"

#include "jsgcinlines.h"
#include "jsinterpinlines.h"

using namespace js;
using namespace js::mjit;

#ifdef __GCC_HAVE_DWARF2_CFI_ASM
# define CFI(str) str
#else
# define CFI(str)
#endif





CFI(asm(".cfi_sections .debug_frame");)

js::mjit::CompilerAllocPolicy::CompilerAllocPolicy(JSContext *cx, Compiler &compiler)
: TempAllocPolicy(cx),
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

extern "C" void JS_FASTCALL
PushActiveVMFrame(VMFrame &f)
{
    f.entryfp->script()->compartment()->jaegerCompartment()->pushActiveFrame(&f);
    f.entryfp->setNativeReturnAddress(JS_FUNC_TO_DATA_PTR(void*, JaegerTrampolineReturn));
    f.regs.clearInlined();
}

extern "C" void JS_FASTCALL
PopActiveVMFrame(VMFrame &f)
{
    f.entryfp->script()->compartment()->jaegerCompartment()->popActiveFrame();
}

extern "C" void JS_FASTCALL
SetVMFrameRegs(VMFrame &f)
{
    f.oldregs = &f.cx->stack.regs();

    
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






JS_STATIC_ASSERT(offsetof(VMFrame, savedRBX) == 0x68);
JS_STATIC_ASSERT(offsetof(VMFrame, scratch) == 0x18);
JS_STATIC_ASSERT(VMFrame::offsetOfFp == 0x38);

JS_STATIC_ASSERT(JSVAL_TAG_MASK == 0xFFFF800000000000LL);
JS_STATIC_ASSERT(JSVAL_PAYLOAD_MASK == 0x00007FFFFFFFFFFFLL);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampoline) "\n"
SYMBOL_STRING(JaegerTrampoline) ":"       "\n"
    
    CFI(".cfi_startproc"                 "\n")
    CFI(".cfi_def_cfa rsp, 8"            "\n")
    "pushq %rbp"                         "\n"
    CFI(".cfi_def_cfa_offset 16"         "\n")
    CFI(".cfi_offset rbp, -16"           "\n")
    "movq %rsp, %rbp"                    "\n"
    CFI(".cfi_def_cfa_register rbp"      "\n")
    
    "pushq %r12"                         "\n"
    CFI(".cfi_offset r12, -24"           "\n")
    "pushq %r13"                         "\n"
    CFI(".cfi_offset r13, -32"           "\n")
    "pushq %r14"                         "\n"
    CFI(".cfi_offset r14, -40"           "\n")
    "pushq %r15"                         "\n"
    CFI(".cfi_offset r15, -48"           "\n")
    "pushq %rbx"                         "\n"
    CFI(".cfi_offset rbx, -56"           "\n")

    
    "movq $0xFFFF800000000000, %r13"     "\n"
    "movq $0x00007FFFFFFFFFFF, %r14"     "\n"

    





    "pushq $0x0"                         "\n" 
    "pushq %rsi"                         "\n" 
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
    CFI(".cfi_endproc"                  "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampolineReturn) "\n"
SYMBOL_STRING(JaegerTrampolineReturn) ":"       "\n"
    CFI(".cfi_startproc"                 "\n")
    CFI(".cfi_def_cfa rbp, 16"           "\n")
    CFI(".cfi_offset rbp, -16"           "\n")
    CFI(".cfi_offset r12, -24"           "\n")
    CFI(".cfi_offset r13, -32"           "\n")
    CFI(".cfi_offset r14, -40"           "\n")
    CFI(".cfi_offset r15, -48"           "\n")
    CFI(".cfi_offset rbx, -56"           "\n")
    "or   %rdi, %rsi"                    "\n"
    "movq %rsi, 0x30(%rbx)"              "\n"
    "movq %rsp, %rdi"                    "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"

    "addq $0x68, %rsp"                   "\n"
    "popq %rbx"                          "\n"
    "popq %r15"                          "\n"
    "popq %r14"                          "\n"
    "popq %r13"                          "\n"
    "popq %r12"                          "\n"
    "popq %rbp"                          "\n"
    CFI(".cfi_def_cfa rsp, 8"            "\n")
    "movq $1, %rax"                      "\n"
    "ret"                                "\n"
    CFI(".cfi_endproc"                   "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerThrowpoline)  "\n"
SYMBOL_STRING(JaegerThrowpoline) ":"        "\n"
    CFI(".cfi_startproc"                    "\n")
    CFI(".cfi_def_cfa rbp, 16"              "\n")
    CFI(".cfi_offset rbp, -16"              "\n")
    CFI(".cfi_offset r12, -24"              "\n")
    CFI(".cfi_offset r13, -32"              "\n")
    CFI(".cfi_offset r14, -40"              "\n")
    CFI(".cfi_offset r15, -48"              "\n")
    CFI(".cfi_offset rbx, -56"              "\n")
    "movq %rsp, %rdi"                       "\n"
    "call " SYMBOL_STRING_RELOC(js_InternalThrow) "\n"
    "testq %rax, %rax"                      "\n"
    "je   throwpoline_exit"                 "\n"
    "jmp  *%rax"                            "\n"
  "throwpoline_exit:"                       "\n"
    "movq %rsp, %rdi"                       "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
    "addq $0x68, %rsp"                      "\n"
    "popq %rbx"                             "\n"
    "popq %r15"                             "\n"
    "popq %r14"                             "\n"
    "popq %r13"                             "\n"
    "popq %r12"                             "\n"
    "popq %rbp"                             "\n"
    CFI(".cfi_def_cfa rsp, 8"               "\n")
    "xorq %rax,%rax"                        "\n"
    "ret"                                   "\n"
    CFI(".cfi_endproc"                      "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerInterpoline)  "\n"
SYMBOL_STRING(JaegerInterpoline) ":"        "\n"
    CFI(".cfi_startproc"                    "\n")
    CFI(".cfi_def_cfa rbp, 16"              "\n")
    CFI(".cfi_offset rbp, -16"              "\n")
    CFI(".cfi_offset r12, -24"              "\n")
    CFI(".cfi_offset r13, -32"              "\n")
    CFI(".cfi_offset r14, -40"              "\n")
    CFI(".cfi_offset r15, -48"              "\n")
    CFI(".cfi_offset rbx, -56"              "\n")
    "movq %rsp, %rcx"                       "\n"
    "movq %rax, %rdx"                       "\n"
    "call " SYMBOL_STRING_RELOC(js_InternalInterpret) "\n"
    "movq 0x38(%rsp), %rbx"                 "\n" 
    "movq 0x30(%rbx), %rsi"                 "\n" 
    "and %r14, %rsi"                        "\n" 
    "movq 0x30(%rbx), %rdi"                 "\n" 
    "and %r13, %rdi"                        "\n" 
    "movq 0x18(%rsp), %rcx"                 "\n" 
    "testq %rax, %rax"                      "\n"
    "je   interpoline_exit"                 "\n"
    "jmp  *%rax"                            "\n"
  "interpoline_exit:"                       "\n"
    "movq %rsp, %rdi"                       "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
    "addq $0x68, %rsp"                      "\n"
    "popq %rbx"                             "\n"
    "popq %r15"                             "\n"
    "popq %r14"                             "\n"
    "popq %r13"                             "\n"
    "popq %r12"                             "\n"
    "popq %rbp"                             "\n"
    CFI(".cfi_def_cfa rsp, 8"               "\n")
    "xorq %rax,%rax"                        "\n"
    "ret"                                   "\n"
    CFI(".cfi_endproc"                      "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerInterpolineScripted)  "\n"
SYMBOL_STRING(JaegerInterpolineScripted) ":"        "\n"
    CFI(".cfi_startproc"                            "\n")
    CFI(".cfi_def_cfa rbp, 16"                      "\n")
    CFI(".cfi_offset rbp, -16"                      "\n")
    CFI(".cfi_offset r12, -24"                      "\n")
    CFI(".cfi_offset r13, -32"                      "\n")
    CFI(".cfi_offset r14, -40"                      "\n")
    CFI(".cfi_offset r15, -48"                      "\n")
    CFI(".cfi_offset rbx, -56"                      "\n")   
    "movq 0x20(%rbx), %rbx"                         "\n" 
    "movq %rbx, 0x38(%rsp)"                         "\n"
    "jmp " SYMBOL_STRING_RELOC(JaegerInterpoline)   "\n"
    CFI(".cfi_endproc"                              "\n")
);

# elif defined(JS_CPU_X86)








JS_STATIC_ASSERT(offsetof(VMFrame, savedEBX) == 0x3C);
JS_STATIC_ASSERT(offsetof(VMFrame, scratch) == 0xC);
JS_STATIC_ASSERT(VMFrame::offsetOfFp == 0x1C);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampoline) "\n"
SYMBOL_STRING(JaegerTrampoline) ":"       "\n"
    
    CFI(".cfi_startproc"                 "\n")
    CFI(".cfi_def_cfa esp, 4"            "\n")
    "pushl %ebp"                         "\n"
    CFI(".cfi_def_cfa_offset 8"          "\n")
    CFI(".cfi_offset ebp, -8"            "\n")
    "movl %esp, %ebp"                    "\n"
    CFI(".cfi_def_cfa_register ebp"      "\n")
    
    "pushl %esi"                         "\n"
    CFI(".cfi_offset esi, -12"           "\n")
    "pushl %edi"                         "\n"
    CFI(".cfi_offset edi, -16"           "\n")
    "pushl %ebx"                         "\n"
    CFI(".cfi_offset ebx, -20"           "\n")

    

    "movl  12(%ebp), %ebx"               "\n"   
    "pushl %ebx"                         "\n"   
    "pushl %ebx"                         "\n"   
    "pushl $0x0"                         "\n"   
    "pushl %ebx"                         "\n"   
    "pushl %ebx"                         "\n"   
    "pushl 20(%ebp)"                     "\n"   
    "pushl 8(%ebp)"                      "\n"   
    "pushl %ebx"                         "\n"   
    "subl $0x1C, %esp"                   "\n"

    
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(SetVMFrameRegs) "\n"
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(PushActiveVMFrame) "\n"

    "movl 28(%esp), %ebp"                "\n"   
    "jmp *88(%esp)"                      "\n"
    CFI(".cfi_endproc"                   "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerTrampolineReturn) "\n"
SYMBOL_STRING(JaegerTrampolineReturn) ":" "\n"
    CFI(".cfi_startproc"                 "\n")
    CFI(".cfi_def_cfa ebp, 8"            "\n")
    CFI(".cfi_offset ebp, -8"            "\n")
    CFI(".cfi_offset esi, -12"           "\n")
    CFI(".cfi_offset edi, -16"           "\n")
    CFI(".cfi_offset ebx, -20"           "\n")
    "movl  %esi, 0x18(%ebp)"             "\n"
    "movl  %edi, 0x1C(%ebp)"             "\n"
    "movl  %esp, %ebp"                   "\n"
    "addl  $0x48, %ebp"                  "\n" 
    "movl  %esp, %ecx"                   "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"

    "addl $0x3C, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    CFI(".cfi_def_cfa esp, 4"            "\n")
    "movl $1, %eax"                      "\n"
    "ret"                                "\n"
    CFI(".cfi_endproc"                   "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerThrowpoline)  "\n"
SYMBOL_STRING(JaegerThrowpoline) ":"        "\n"
    
    CFI(".cfi_startproc"                 "\n")
    CFI(".cfi_def_cfa ebp, 8"            "\n")
    CFI(".cfi_offset ebp, -8"            "\n")
    CFI(".cfi_offset esi, -12"           "\n")
    CFI(".cfi_offset edi, -16"           "\n")
    CFI(".cfi_offset ebx, -20"           "\n")
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
    "addl $0x3c, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    CFI(".cfi_def_cfa esp, 4"            "\n")
    "xorl %eax, %eax"                    "\n"
    "ret"                                "\n"
    CFI(".cfi_endproc"                   "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerInterpoline)  "\n"
SYMBOL_STRING(JaegerInterpoline) ":"        "\n"
    CFI(".cfi_startproc"                 "\n")
    CFI(".cfi_def_cfa ebp, 8"            "\n")
    CFI(".cfi_offset ebp, -8"            "\n")
    CFI(".cfi_offset esi, -12"           "\n")
    CFI(".cfi_offset edi, -16"           "\n")
    CFI(".cfi_offset ebx, -20"           "\n")
    
    "pushl %esp"                         "\n"
    "pushl %eax"                         "\n"
    "pushl %edi"                         "\n"
    "pushl %esi"                         "\n"
    "call " SYMBOL_STRING_RELOC(js_InternalInterpret) "\n"
    "addl $0x10, %esp"                   "\n"
    "movl 0x1C(%esp), %ebp"              "\n" 
    "movl 0x18(%ebp), %esi"              "\n" 
    "movl 0x1C(%ebp), %edi"              "\n" 
    "movl 0xC(%esp), %ecx"               "\n" 
    "testl %eax, %eax"                   "\n"
    "je   interpoline_exit"              "\n"
    "jmp  *%eax"                         "\n"
  "interpoline_exit:"                    "\n"
    "movl %esp, %ecx"                    "\n"
    "call " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
    "addl $0x3c, %esp"                   "\n"
    "popl %ebx"                          "\n"
    "popl %edi"                          "\n"
    "popl %esi"                          "\n"
    "popl %ebp"                          "\n"
    CFI(".cfi_def_cfa esp, 4"            "\n")
    "xorl %eax, %eax"                    "\n"
    "ret"                                "\n"
    CFI(".cfi_endproc"                   "\n")
);

asm (
".text\n"
".globl " SYMBOL_STRING(JaegerInterpolineScripted)  "\n"
SYMBOL_STRING(JaegerInterpolineScripted) ":"        "\n"
    CFI(".cfi_startproc"                            "\n")
    CFI(".cfi_def_cfa ebp, 8"                       "\n")
    CFI(".cfi_offset ebp, -8"                       "\n")
    CFI(".cfi_offset esi, -12"                      "\n")
    CFI(".cfi_offset edi, -16"                      "\n")
    CFI(".cfi_offset ebx, -20"                      "\n")      
    "movl 0x10(%ebp), %ebp"                         "\n" 
    "movl  %ebp, 0x1C(%esp)"                        "\n"
    "jmp " SYMBOL_STRING_RELOC(JaegerInterpoline)   "\n"
    CFI(".cfi_endproc"                              "\n")
);

# elif defined(JS_CPU_ARM)

JS_STATIC_ASSERT(sizeof(VMFrame) == 88);
JS_STATIC_ASSERT(sizeof(VMFrame)%8 == 0);   
JS_STATIC_ASSERT(offsetof(VMFrame, savedLR) ==          (4*21));
JS_STATIC_ASSERT(offsetof(VMFrame, entryfp) ==          (4*10));
JS_STATIC_ASSERT(offsetof(VMFrame, stackLimit) ==       (4*9));
JS_STATIC_ASSERT(offsetof(VMFrame, cx) ==               (4*8));
JS_STATIC_ASSERT(VMFrame::offsetOfFp ==                 (4*7));
JS_STATIC_ASSERT(offsetof(VMFrame, scratch) ==          (4*3));
JS_STATIC_ASSERT(offsetof(VMFrame, previous) ==         (4*2));

JS_STATIC_ASSERT(JSFrameReg == JSC::ARMRegisters::r10);
JS_STATIC_ASSERT(JSReturnReg_Type == JSC::ARMRegisters::r5);
JS_STATIC_ASSERT(JSReturnReg_Data == JSC::ARMRegisters::r4);

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
    
"   mov     ip, #0"                             "\n"    
"   push    {ip}"                               "\n"    
"   push    {r1}"                               "\n"    
"   push    {r1}"                               "\n"    
"   push    {r3}"                               "\n"    
"   push    {r0}"                               "\n"    
"   push    {r1}"                               "\n"    
    
"   sub     sp, sp, #(4*7)"                     "\n"

    
"   mov     r4, r2"                             "\n"
    
"   mov     r10, r1"                            "\n"

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
"   strd    r4, r5, [r10, #24]"             "\n" 

    
"   mov     r0, sp"                         "\n"
"   blx  " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"

    
"   add     sp, sp, #(4*7 + 4*6)"           "\n"

    
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

    
"   mov     r0, sp"                         "\n"
"   blx  " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
"   add     sp, sp, #(4*7 + 4*6)"           "\n"
"   mov     r0, #0"                         "\n"
"   pop     {r4-r11,pc}"                    "\n"
);

asm (
".text\n"
FUNCTION_HEADER_EXTRA
".globl " SYMBOL_STRING(JaegerInterpolineScripted)  "\n"
SYMBOL_STRING(JaegerInterpolineScripted) ":"        "\n"
    

"   ldr     r10, [r10, #(4*4)]"             "\n"    
"   str     r10, [sp, #(4*7)]"              "\n"    
    

FUNCTION_HEADER_EXTRA
".globl " SYMBOL_STRING(JaegerInterpoline)  "\n"
SYMBOL_STRING(JaegerInterpoline) ":"        "\n"
"   mov     r3, sp"                         "\n"    
"   mov     r2, r0"                         "\n"    
"   mov     r1, r5"                         "\n"    
"   mov     r0, r4"                         "\n"    
"   blx  " SYMBOL_STRING_RELOC(js_InternalInterpret) "\n"
"   cmp     r0, #0"                         "\n"
"   ldr     r10, [sp, #(4*7)]"              "\n"    
"   ldrd    r4, r5, [r10, #(4*6)]"          "\n"    
"   ldr     r1, [sp, #(4*3)]"               "\n"    
"   it      ne"                             "\n"
"   bxne    r0"                             "\n"
    
"   mov     r0, sp"                         "\n"
"   blx  " SYMBOL_STRING_VMFRAME(PopActiveVMFrame) "\n"
"   add     sp, sp, #(4*7 + 4*6)"           "\n"
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








JS_STATIC_ASSERT(offsetof(VMFrame, savedEBX) == 0x3C);
JS_STATIC_ASSERT(offsetof(VMFrame, scratch) == 0xC);
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
            push ebx;
            push 0x0;
            push ebx;
            push ebx;
            push [ebp + 20];
            push [ebp + 8];
            push ebx;
            sub  esp, 0x1C;

            
            mov  ecx, esp;
            call SetVMFrameRegs;
            mov  ecx, esp;
            call PushActiveVMFrame;

            mov ebp, [esp + 28];  
            jmp dword ptr [esp + 88];
        }
    }

    __declspec(naked) void JaegerTrampolineReturn()
    {
        __asm {
            mov [ebp + 0x18], esi;
            mov [ebp + 0x1C], edi;
            mov  ebp, esp;
            add  ebp, 0x48; 
            mov  ecx, esp;
            call PopActiveVMFrame;

            add esp, 0x3C;

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
            add esp, 0x3c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            xor eax, eax
            ret;
        }
    }

    extern "C" void *
    js_InternalInterpret(void *returnData, void *returnType, void *returnReg, js::VMFrame &f);

    __declspec(naked) void JaegerInterpoline() {
        __asm {
            
            push esp;
            push eax;
            push edi;
            push esi;
            call js_InternalInterpret;
            add esp, 0x10;
            mov ebp, [esp + 0x1C];  
            mov esi, [ebp + 0x18];  
            mov edi, [ebp + 0x1C];  
            mov ecx, [esp + 0xC];   
            test eax, eax;
            je interpoline_exit;
            jmp eax;
        interpoline_exit:
            mov ecx, esp;
            call PopActiveVMFrame;
            add esp, 0x3c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            xor eax, eax
            ret;
        }
    }

    __declspec(naked) void JaegerInterpolineScripted() {
        __asm {
            mov ebp, [ebp + 0x10];  
            mov [esp + 0x1C], ebp;  
            jmp JaegerInterpoline;
        }
    }
}



#elif defined(_WIN64)






JS_STATIC_ASSERT(offsetof(VMFrame, savedRBX) == 0x68);
JS_STATIC_ASSERT(offsetof(VMFrame, scratch) == 0x18);
JS_STATIC_ASSERT(VMFrame::offsetOfFp == 0x38);
JS_STATIC_ASSERT(JSVAL_TAG_MASK == 0xFFFF800000000000LL);
JS_STATIC_ASSERT(JSVAL_PAYLOAD_MASK == 0x00007FFFFFFFFFFFLL);

#endif                   

JaegerCompartment::JaegerCompartment()
    : orphanedNativeFrames(SystemAllocPolicy()), orphanedNativePools(SystemAllocPolicy())
{}

bool
JaegerCompartment::Initialize()
{
    execAlloc_ = js::OffTheBooks::new_<JSC::ExecutableAllocator>();
    if (!execAlloc_)
        return false;
    
    TrampolineCompiler tc(execAlloc_, &trampolines);
    if (!tc.compile()) {
        js::Foreground::delete_(execAlloc_);
        execAlloc_ = NULL;
        return false;
    }

#ifdef JS_METHODJIT_PROFILE_STUBS
    for (size_t i = 0; i < STUB_CALLS_FOR_OP_COUNT; ++i)
        StubCallsForOp[i] = 0;
#endif

    activeFrame_ = NULL;
    lastUnfinished_ = (JaegerStatus) 0;

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

JaegerStatus
mjit::EnterMethodJIT(JSContext *cx, StackFrame *fp, void *code, Value *stackLimit, bool partial)
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

#ifdef JS_METHODJIT_SPEW
    prof.stop();
    JaegerSpew(JSpew_Prof, "script run took %d ms\n", prof.time_ms());
#endif

    
    cx->stack.repointRegs(&oldRegs);

    JaegerStatus status = cx->compartment->jaegerCompartment()->lastUnfinished();
    if (status) {
        if (partial) {
            



            return status;
        }

        





        InterpMode mode = (status == Jaeger_UnfinishedAtTrap)
            ? JSINTERP_SKIP_TRAP
            : JSINTERP_REJOIN;
        ok = Interpret(cx, fp, mode);

        return ok ? Jaeger_Returned : Jaeger_Throwing;
    }

    
    JS_ASSERT(fp == cx->fp());

    if (ok) {
        
        fp->markReturnValue();
    }

    
    if (fp->isFunctionFrame())
        fp->markFunctionEpilogueDone();

    return ok ? Jaeger_Returned : Jaeger_Throwing;
}

static inline JaegerStatus
CheckStackAndEnterMethodJIT(JSContext *cx, StackFrame *fp, void *code, bool partial)
{
    JS_CHECK_RECURSION(cx, return Jaeger_Throwing);

    JS_ASSERT(!cx->compartment->activeAnalysis);

    Value *stackLimit = cx->stack.space().getStackLimit(cx, REPORT_ERROR);
    if (!stackLimit)
        return Jaeger_Throwing;

    return EnterMethodJIT(cx, fp, code, stackLimit, partial);
}

JaegerStatus
mjit::JaegerShot(JSContext *cx, bool partial)
{
    StackFrame *fp = cx->fp();
    JSScript *script = fp->script();
    JITScript *jit = script->getJIT(fp->isConstructing());

#ifdef JS_TRACER
    if (TRACE_RECORDER(cx))
        AbortRecording(cx, "attempt to enter method JIT while recording");
#endif

    JS_ASSERT(cx->regs().pc == script->code);

    return CheckStackAndEnterMethodJIT(cx, cx->fp(), jit->invokeEntry, partial);
}

JaegerStatus
js::mjit::JaegerShotAtSafePoint(JSContext *cx, void *safePoint, bool partial)
{
#ifdef JS_TRACER
    JS_ASSERT(!TRACE_RECORDER(cx));
#endif

    return CheckStackAndEnterMethodJIT(cx, cx->fp(), safePoint, partial);
}

NativeMapEntry *
JITScript::nmap() const
{
    return (NativeMapEntry *)((char*)this + sizeof(JITScript));
}

js::mjit::InlineFrame *
JITScript::inlineFrames() const
{
    return (js::mjit::InlineFrame *)((char *)nmap() + sizeof(NativeMapEntry) * nNmapPairs);
}

js::mjit::CallSite *
JITScript::callSites() const
{
    return (js::mjit::CallSite *)&inlineFrames()[nInlineFrames];
}

JSObject **
JITScript::rootedObjects() const
{
    return (JSObject **)&callSites()[nCallSites];
}

char *
JITScript::commonSectionLimit() const
{
    return (char *)&rootedObjects()[nRootedObjects];
}

#ifdef JS_MONOIC
ic::GetGlobalNameIC *
JITScript::getGlobalNames() const
{
    return (ic::GetGlobalNameIC *) commonSectionLimit();
}

ic::SetGlobalNameIC *
JITScript::setGlobalNames() const
{
    return (ic::SetGlobalNameIC *)((char *)getGlobalNames() +
            sizeof(ic::GetGlobalNameIC) * nGetGlobalNames);
}

ic::CallICInfo *
JITScript::callICs() const
{
    return (ic::CallICInfo *)&setGlobalNames()[nSetGlobalNames];
}

ic::EqualityICInfo *
JITScript::equalityICs() const
{
    return (ic::EqualityICInfo *)&callICs()[nCallICs];
}

ic::TraceICInfo *
JITScript::traceICs() const
{
    return (ic::TraceICInfo *)&equalityICs()[nEqualityICs];
}

char *
JITScript::monoICSectionsLimit() const
{
    return (char *)&traceICs()[nTraceICs];
}
#else   
char *
JITScript::monoICSectionsLimit() const
{
    return commonSectionLimit();
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

template <typename T>
static inline void Destroy(T &t)
{
    t.~T();
}

void
mjit::JITScript::purgeNativeCallStubs()
{
    for (unsigned i = 0; i < nativeCallStubs.length(); i++) {
        JSC::ExecutablePool *pool = nativeCallStubs[i].pool;
        if (pool)
            pool->release();
    }
    nativeCallStubs.clear();
}

mjit::JITScript::~JITScript()
{
    code.release();

    if (pcLengths)
        Foreground::free_(pcLengths);

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
    if (argsCheckPool)
        argsCheckPool->release();

    for (JSC::ExecutablePool **pExecPool = execPools.begin();
         pExecPool != execPools.end();
         ++pExecPool)
    {
        (*pExecPool)->release();
    }

    purgeNativeCallStubs();

    ic::CallICInfo *callICs_ = callICs();
    for (uint32 i = 0; i < nCallICs; i++) {
        callICs_[i].releasePools();
        if (callICs_[i].fastGuardedObject)
            callICs_[i].purgeGuardedObject();
    }

    
    while (!JS_CLIST_IS_EMPTY(&callers)) {
        JS_STATIC_ASSERT(offsetof(ic::CallICInfo, links) == 0);
        ic::CallICInfo *ic = (ic::CallICInfo *) callers.next;

        uint8 *start = (uint8 *)ic->funGuard.executableAddress();
        JSC::RepatchBuffer repatch(JSC::JITCode(start - 32, 64));

        repatch.repatch(ic->funGuard, NULL);
        repatch.relink(ic->funJump, ic->slowPathStart);
        ic->purgeGuardedObject();
    }
#endif
}

size_t
JSScript::jitDataSize(JSUsableSizeFun usf)
{
    size_t n = 0;
    if (jitNormal)
        n += jitNormal->scriptDataSize(usf); 
    if (jitCtor)
        n += jitCtor->scriptDataSize(usf); 
    return n;
}


size_t
mjit::JITScript::scriptDataSize(JSUsableSizeFun usf)
{
    size_t usable = usf ? usf(this) : 0;
    return usable ? usable :
        sizeof(JITScript) +
        sizeof(NativeMapEntry) * nNmapPairs +
        sizeof(InlineFrame) * nInlineFrames +
        sizeof(CallSite) * nCallSites +
        sizeof(JSObject *) * nRootedObjects +
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
        0;
}

void
mjit::ReleaseScriptCode(JSContext *cx, JSScript *script, bool construct)
{
    
    
    

    JITScript **pjit = construct ? &script->jitCtor : &script->jitNormal;
    void **parity = construct ? &script->jitArityCheckCtor : &script->jitArityCheckNormal;

    if (*pjit) {
        Probes::discardMJITCode(cx, *pjit, script, (*pjit)->code.m_code.executableAddress());
        (*pjit)->~JITScript();
        cx->free_(*pjit);
        *pjit = NULL;
        *parity = NULL;
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
JITScript::nativeToPC(void *returnAddress, CallSite **pinline) const
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

    if (ic.call->inlineIndex != uint32(-1)) {
        if (pinline)
            *pinline = ic.call;
        InlineFrame *frame = &inlineFrames()[ic.call->inlineIndex];
        while (frame && frame->parent)
            frame = frame->parent;
        return frame->parentpc;
    }

    if (pinline)
        *pinline = NULL;
    return script->code + ic.call->pcOffset;
}

jsbytecode *
mjit::NativeToPC(JITScript *jit, void *ncode, mjit::CallSite **pinline)
{
    return jit->nativeToPC(ncode, pinline);
}

void
JITScript::trace(JSTracer *trc)
{
    




    InlineFrame *inlineFrames_ = inlineFrames();
    for (unsigned i = 0; i < nInlineFrames; i++)
        MarkObject(trc, *inlineFrames_[i].fun, "jitscript_fun");

    for (uint32 i = 0; i < nRootedObjects; ++i)
        MarkObject(trc, *rootedObjects()[i], "mjit rooted object");
}

 const double mjit::Assembler::oneDouble = 1.0;
