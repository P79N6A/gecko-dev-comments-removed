






#define ENABLE_ASSEMBLER 1


#define ENABLE_JIT 1

#define USE_SYSTEM_MALLOC 1


#include <jit/ExecutableAllocator.h>
#include <assembler/LinkBuffer.h>
#include <assembler/CodeLocation.h>
#include <assembler/RepatchBuffer.h>

#include <assembler/MacroAssembler.h>

#include <stdio.h>



#undef ARCH_x86
#undef ARCH_amd64
#undef ARCH_arm

#if defined(__APPLE__) && defined(__i386__)
#  define ARCH_x86 1
#elif defined(__APPLE__) && defined(__x86_64__)
#  define ARCH_amd64 1
#elif defined(__linux__) && defined(__i386__)
#  define ARCH_x86 1
#elif defined(__linux__) && defined(__x86_64__)
#  define ARCH_amd64 1
#elif defined(__linux__) && defined(__arm__)
#  define ARCH_arm 1
#elif defined(_MSC_VER) && defined(_M_IX86)
#  define ARCH_x86 1
#endif




#if WTF_COMPILER_GCC
__attribute__((noinline))
#endif
void pre_run ( void ) { }



#if WTF_COMPILER_GCC

void test1 ( void )
{
  printf("\n------------ Test 1 (straight line code) ------------\n\n" );

  
  JSC::MacroAssembler* am = new JSC::MacroAssembler();

#if defined(ARCH_amd64)
  JSC::X86Registers::RegisterID areg = JSC::X86Registers::r15;
  
  
  
  
  
  am->xorPtr(areg,areg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), areg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), areg);
  am->ret();
#endif

#if defined(ARCH_x86)
  JSC::X86Registers::RegisterID areg = JSC::X86Registers::edi;
  
  
  
  
  
  am->xorPtr(areg,areg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), areg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), areg);
  am->ret();
#endif

#if defined(ARCH_arm)
  JSC::ARMRegisters::RegisterID areg = JSC::ARMRegisters::r8;
  
  
  
  
  
  
  am->xorPtr(areg,areg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), areg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), areg);
  am->ret();
#endif

  
  JSC::ExecutableAllocator* eal = new JSC::ExecutableAllocator();

  
  
  JSC::ExecutablePool* ep = eal->poolForSize( am->size() );

  
  
  JSC::LinkBuffer patchBuffer(am, ep);

  
  JSC::MacroAssemblerCodeRef cr = patchBuffer.finalizeCode();

  
  void* entry = cr.m_code.executableAddress();

  printf("disas %p %p\n",
         entry, (char*)entry + cr.m_size);
  pre_run();

  unsigned long result = 0x55555555;

#if defined(ARCH_amd64)
  
  __asm__ __volatile__(
     "callq *%1"           "\n\t"
     "movq  %%r15, %0"     "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "r15","cc"
  );
#endif
#if defined(ARCH_x86)
  
  __asm__ __volatile__(
     "calll *%1"           "\n\t"
     "movl  %%edi, %0"     "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "edi","cc"
  );
#endif
#if defined(ARCH_arm)
  
  __asm__ __volatile__(
     "blx   %1"            "\n\t"
     "mov   %0, %%r8"      "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "r8","cc"
  );
#endif

  printf("\n");
  printf("value computed is %lu (expected 444)\n", result);
  printf("\n");

  delete eal;
  delete am;
}

#endif 



#if WTF_COMPILER_GCC

void test2 ( void )
{
  printf("\n------------ Test 2 (mini loop) ------------\n\n" );

  
  JSC::MacroAssembler* am = new JSC::MacroAssembler();

#if defined(ARCH_amd64)
  JSC::X86Registers::RegisterID areg = JSC::X86Registers::r15;
  
  
  
  
  
  
  
  
  
  am->xorPtr(areg,areg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), areg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), areg);

  JSC::MacroAssembler::Label loopHeadLabel(am);
  am->subPtr(JSC::MacroAssembler::Imm32(1), areg);

  JSC::MacroAssembler::Jump j
     = am->branchPtr(JSC::MacroAssembler::NotEqual,
                     areg, JSC::MacroAssembler::ImmPtr(0));
  j.linkTo(loopHeadLabel, am);

  am->ret();
#endif

#if defined(ARCH_x86)
  JSC::X86Registers::RegisterID areg = JSC::X86Registers::edi;
  
  
  
  
  
  
  
  
  am->xorPtr(areg,areg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), areg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), areg);

  JSC::MacroAssembler::Label loopHeadLabel(am);
  am->subPtr(JSC::MacroAssembler::Imm32(1), areg);

  JSC::MacroAssembler::Jump j
     = am->branchPtr(JSC::MacroAssembler::NotEqual,
                     areg, JSC::MacroAssembler::ImmPtr(0));
  j.linkTo(loopHeadLabel, am);

  am->ret();
#endif

#if defined(ARCH_arm)
  JSC::ARMRegisters::RegisterID areg = JSC::ARMRegisters::r8;
  
  
  
  
  
  
  
  
  
  
  
  
  
  am->xorPtr(areg,areg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), areg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), areg);

  JSC::MacroAssembler::Label loopHeadLabel(am);
  am->subPtr(JSC::MacroAssembler::Imm32(1), areg);

  JSC::MacroAssembler::Jump j
     = am->branchPtr(JSC::MacroAssembler::NotEqual,
                     areg, JSC::MacroAssembler::ImmPtr(0));
  j.linkTo(loopHeadLabel, am);

  am->ret();
#endif

  
  JSC::ExecutableAllocator* eal = new JSC::ExecutableAllocator();

  
  
  JSC::ExecutablePool* ep = eal->poolForSize( am->size() );

  
  
  JSC::LinkBuffer patchBuffer(am, ep);

  
  JSC::MacroAssemblerCodeRef cr = patchBuffer.finalizeCode();

  
  void* entry = cr.m_code.executableAddress();

  printf("disas %p %p\n",
         entry, (char*)entry + cr.m_size);
  pre_run();

  unsigned long result = 0x55555555;

#if defined(ARCH_amd64)
  
  __asm__ __volatile__(
     "callq *%1"           "\n\t"
     "movq  %%r15, %0"     "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "r15","cc"
  );
#endif
#if defined(ARCH_x86)
  
  __asm__ __volatile__(
     "calll *%1"           "\n\t"
     "movl  %%edi, %0"     "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "edi","cc"
  );
#endif
#if defined(ARCH_arm)
  
  __asm__ __volatile__(
     "blx   %1"            "\n\t"
     "mov   %0, %%r8"      "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "r8","cc"
  );
#endif

  printf("\n");
  printf("value computed is %lu (expected 0)\n", result);
  printf("\n");

  delete eal;
  delete am;
}

#endif 



#if WTF_COMPILER_GCC

void test3 ( void )
{
  printf("\n------------ Test 3 (if-then-else) ------------\n\n" );

  
  JSC::MacroAssembler* am = new JSC::MacroAssembler();

#if defined(ARCH_amd64)
  JSC::X86Registers::RegisterID areg = JSC::X86Registers::r15;
  
  
  
  
  
  
  
  
  

  
  am->move(JSC::MacroAssembler::Imm32(100), areg);

  
  JSC::MacroAssembler::Jump jToElse
    = am->branchPtr(JSC::MacroAssembler::NotEqual,
                    areg, JSC::MacroAssembler::ImmPtr(0));

  
  am->move(JSC::MacroAssembler::Imm32(64), areg);
  JSC::MacroAssembler::Jump jToAfter
    = am->jump();

  
  JSC::MacroAssembler::Label elseLbl(am);
  am->move(JSC::MacroAssembler::Imm32(4), areg);

  
  JSC::MacroAssembler::Label afterLbl(am);

  am->ret();
#endif

#if defined(ARCH_x86)
  JSC::X86Registers::RegisterID areg = JSC::X86Registers::edi;
  
  
  
  
  
  
  
  

  
  am->move(JSC::MacroAssembler::Imm32(100), areg);

  
  JSC::MacroAssembler::Jump jToElse
    = am->branchPtr(JSC::MacroAssembler::NotEqual,
                    areg, JSC::MacroAssembler::ImmPtr(0));

  
  am->move(JSC::MacroAssembler::Imm32(64), areg);
  JSC::MacroAssembler::Jump jToAfter
    = am->jump();

  
  JSC::MacroAssembler::Label elseLbl(am);
  am->move(JSC::MacroAssembler::Imm32(4), areg);

  
  JSC::MacroAssembler::Label afterLbl(am);

  am->ret();
#endif

#if defined(ARCH_arm)
  JSC::ARMRegisters::RegisterID areg = JSC::ARMRegisters::r8;
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  am->move(JSC::MacroAssembler::Imm32(100), areg);

  
  JSC::MacroAssembler::Jump jToElse
    = am->branchPtr(JSC::MacroAssembler::NotEqual,
                    areg, JSC::MacroAssembler::ImmPtr(0));

  
  am->move(JSC::MacroAssembler::Imm32(64), areg);
  JSC::MacroAssembler::Jump jToAfter
    = am->jump();

  
  JSC::MacroAssembler::Label elseLbl(am);
  am->move(JSC::MacroAssembler::Imm32(4), areg);

  
  JSC::MacroAssembler::Label afterLbl(am);

  am->ret();
#endif

  
  jToElse.linkTo(elseLbl, am);
  jToAfter.linkTo(afterLbl, am);

  
  JSC::ExecutableAllocator* eal = new JSC::ExecutableAllocator();

  
  
  JSC::ExecutablePool* ep = eal->poolForSize( am->size() );

  
  
  JSC::LinkBuffer patchBuffer(am, ep);

  
  JSC::MacroAssemblerCodeRef cr = patchBuffer.finalizeCode();

  
  void* entry = cr.m_code.executableAddress();

  printf("disas %p %p\n",
         entry, (char*)entry + cr.m_size);
  pre_run();

  unsigned long result = 0x55555555;

#if defined(ARCH_amd64)
  
  __asm__ __volatile__(
     "callq *%1"           "\n\t"
     "movq  %%r15, %0"     "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "r15","cc"
  );
#endif
#if defined(ARCH_x86)
  
  __asm__ __volatile__(
     "calll *%1"           "\n\t"
     "movl  %%edi, %0"     "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "edi","cc"
  );
#endif
#if defined(ARCH_arm)
  
  __asm__ __volatile__(
     "blx   %1"            "\n\t"
     "mov   %0, %%r8"      "\n"
     :   "=r"(result)
     :    "r"(entry)
     : "r8","cc"
  );
#endif

  printf("\n");
  printf("value computed is %lu (expected 4)\n", result);
  printf("\n");

  delete eal;
  delete am;
}

#endif 




void test4 ( void )
{
  printf("\n------------ Test 4 (callable fn) ------------\n\n" );

  
  JSC::MacroAssembler* am = new JSC::MacroAssembler();

#if defined(ARCH_amd64)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  JSC::X86Registers::RegisterID rreg = JSC::X86Registers::eax;
  am->push(JSC::X86Registers::ebp);
  am->move(JSC::X86Registers::esp, JSC::X86Registers::ebp);
  am->push(JSC::X86Registers::ebx);
  am->push(JSC::X86Registers::r12);
  am->push(JSC::X86Registers::r13);
  am->push(JSC::X86Registers::r14);
  am->push(JSC::X86Registers::r15);

  am->xorPtr(rreg,rreg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), rreg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), rreg);

  am->pop(JSC::X86Registers::r15);
  am->pop(JSC::X86Registers::r14);
  am->pop(JSC::X86Registers::r13);
  am->pop(JSC::X86Registers::r12);
  am->pop(JSC::X86Registers::ebx);
  am->move(JSC::X86Registers::ebp, JSC::X86Registers::esp);
  am->pop(JSC::X86Registers::ebp);
  am->ret();
#endif

#if defined(ARCH_x86)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  JSC::X86Registers::RegisterID rreg = JSC::X86Registers::eax;

  am->push(JSC::X86Registers::ebp);
  am->move(JSC::X86Registers::esp, JSC::X86Registers::ebp);
  am->push(JSC::X86Registers::ebx);
  am->push(JSC::X86Registers::esi);
  am->push(JSC::X86Registers::edi);

  am->xorPtr(rreg,rreg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), rreg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), rreg);

  am->pop(JSC::X86Registers::edi);
  am->pop(JSC::X86Registers::esi);
  am->pop(JSC::X86Registers::ebx);
  am->move(JSC::X86Registers::ebp, JSC::X86Registers::esp);
  am->pop(JSC::X86Registers::ebp);
  am->ret();
#endif

#if defined(ARCH_arm)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  JSC::ARMRegisters::RegisterID rreg = JSC::ARMRegisters::r0;

  am->push(JSC::ARMRegisters::r4);
  am->push(JSC::ARMRegisters::r5);
  am->push(JSC::ARMRegisters::r6);
  am->push(JSC::ARMRegisters::r7);
  am->push(JSC::ARMRegisters::r8);
  am->push(JSC::ARMRegisters::r9);
  am->push(JSC::ARMRegisters::r10);
  am->push(JSC::ARMRegisters::r11);

  am->xorPtr(rreg,rreg);
  am->addPtr(JSC::MacroAssembler::Imm32(123), rreg);
  am->addPtr(JSC::MacroAssembler::Imm32(321), rreg);

  am->pop(JSC::ARMRegisters::r11);
  am->pop(JSC::ARMRegisters::r10);
  am->pop(JSC::ARMRegisters::r9);
  am->pop(JSC::ARMRegisters::r8);
  am->pop(JSC::ARMRegisters::r7);
  am->pop(JSC::ARMRegisters::r6);
  am->pop(JSC::ARMRegisters::r5);
  am->pop(JSC::ARMRegisters::r4);

  am->ret();
#endif

  
  JSC::ExecutableAllocator* eal = new JSC::ExecutableAllocator();

  
  
  JSC::ExecutablePool* ep = eal->poolForSize( am->size() );

  
  
  JSC::LinkBuffer patchBuffer(am, ep);

  
  

  
  JSC::MacroAssemblerCodeRef cr = patchBuffer.finalizeCode();

  
  void* entry = cr.m_code.executableAddress();

  printf("disas %p %p\n",
         entry, (char*)entry + cr.m_size);
  pre_run();

  
  unsigned long (*fn)(void) = (unsigned long (*)())entry;
  unsigned long result = fn();

  printf("\n");
  printf("value computed is %lu (expected 444)\n", result);
  printf("\n");

  delete eal;
  delete am;
}






unsigned long cube   ( unsigned long x ) { return x * x * x; }
unsigned long square ( unsigned long x ) { return x * x; }

void test5 ( void )
{
  printf("\n--------- Test 5 (call in, out, repatch) ---------\n\n" );

  
  JSC::MacroAssembler* am = new JSC::MacroAssembler();
  JSC::MacroAssembler::Call cl;
  ptrdiff_t offset_of_call_insn;

#if defined(ARCH_amd64)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  JSC::MacroAssembler::Label startOfFnLbl(am);
  am->push(JSC::X86Registers::ebp);
  am->move(JSC::X86Registers::esp, JSC::X86Registers::ebp);
  am->push(JSC::X86Registers::ebx);
  am->push(JSC::X86Registers::r12);
  am->push(JSC::X86Registers::r13);
  am->push(JSC::X86Registers::r14);
  am->push(JSC::X86Registers::r15);

  
  am->move(JSC::MacroAssembler::Imm32(9), JSC::X86Registers::edi);
  cl = am->JSC::MacroAssembler::call();

  

  am->pop(JSC::X86Registers::r15);
  am->pop(JSC::X86Registers::r14);
  am->pop(JSC::X86Registers::r13);
  am->pop(JSC::X86Registers::r12);
  am->pop(JSC::X86Registers::ebx);
  am->move(JSC::X86Registers::ebp, JSC::X86Registers::esp);
  am->pop(JSC::X86Registers::ebp);
  am->ret();

  offset_of_call_insn
     = am->JSC::MacroAssembler::differenceBetween(startOfFnLbl, cl);
  if (0) printf("XXXXXXXX offset = %lu\n", offset_of_call_insn);
#endif

#if defined(ARCH_x86)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  JSC::MacroAssembler::Label startOfFnLbl(am);
  am->push(JSC::X86Registers::ebp);
  am->move(JSC::X86Registers::esp, JSC::X86Registers::ebp);
  am->push(JSC::X86Registers::ebx);
  am->push(JSC::X86Registers::esi);
  am->push(JSC::X86Registers::edi);

  
  am->push(JSC::MacroAssembler::Imm32(9));
  cl = am->JSC::MacroAssembler::call();
  am->addPtr(JSC::MacroAssembler::Imm32(4), JSC::X86Registers::esp);
  

  am->pop(JSC::X86Registers::edi);
  am->pop(JSC::X86Registers::esi);
  am->pop(JSC::X86Registers::ebx);
  am->move(JSC::X86Registers::ebp, JSC::X86Registers::esp);
  am->pop(JSC::X86Registers::ebp);
  am->ret();

  offset_of_call_insn
     = am->JSC::MacroAssembler::differenceBetween(startOfFnLbl, cl);
  if (0) printf("XXXXXXXX offset = %lu\n",
                (unsigned long)offset_of_call_insn);
#endif

#if defined(ARCH_arm)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  JSC::MacroAssembler::Label startOfFnLbl(am);
  am->push(JSC::ARMRegisters::r4);
  am->push(JSC::ARMRegisters::r5);
  am->push(JSC::ARMRegisters::r6);
  am->push(JSC::ARMRegisters::r7);
  am->push(JSC::ARMRegisters::r8);
  am->push(JSC::ARMRegisters::r9);
  am->push(JSC::ARMRegisters::r10);
  am->push(JSC::ARMRegisters::r11);
  am->push(JSC::ARMRegisters::lr);

  
  am->move(JSC::MacroAssembler::Imm32(9), JSC::ARMRegisters::r0);
  cl = am->JSC::MacroAssembler::call();
  

  am->pop(JSC::ARMRegisters::lr);
  am->pop(JSC::ARMRegisters::r11);
  am->pop(JSC::ARMRegisters::r10);
  am->pop(JSC::ARMRegisters::r9);
  am->pop(JSC::ARMRegisters::r8);
  am->pop(JSC::ARMRegisters::r7);
  am->pop(JSC::ARMRegisters::r6);
  am->pop(JSC::ARMRegisters::r5);
  am->pop(JSC::ARMRegisters::r4);
  am->ret();

  offset_of_call_insn
     = am->JSC::MacroAssembler::differenceBetween(startOfFnLbl, cl);
  if (0) printf("XXXXXXXX offset = %lu\n",
                (unsigned long)offset_of_call_insn);
#endif

  
  JSC::ExecutableAllocator* eal = new JSC::ExecutableAllocator();

  
  
  JSC::ExecutablePool* ep = eal->poolForSize( am->size() );

  
  
  JSC::LinkBuffer patchBuffer(am, ep);

  
  JSC::FunctionPtr target = JSC::FunctionPtr::FunctionPtr( &cube );
  patchBuffer.link(cl, target);

  JSC::MacroAssemblerCodeRef cr = patchBuffer.finalizeCode();

  
  void* entry = cr.m_code.executableAddress();

  printf("disas %p %p\n",
         entry, (char*)entry + cr.m_size);


  pre_run();

  printf("\n");

  unsigned long (*fn)() = (unsigned long(*)())entry;
  unsigned long result = fn();

  printf("value computed is %lu (expected 729)\n", result);
  printf("\n");

  
  JSC::JITCode jc = JSC::JITCode::JITCode(entry, cr.m_size);
  JSC::CodeBlock cb = JSC::CodeBlock::CodeBlock(jc);

  
  JSC::MacroAssemblerCodePtr cp
     = JSC::MacroAssemblerCodePtr( ((char*)entry) + offset_of_call_insn );

  JSC::RepatchBuffer repatchBuffer(&cb);
  repatchBuffer.relink( JSC::CodeLocationCall(cp),
                        JSC::FunctionPtr::FunctionPtr( &square ));
 
  result = fn();
  printf("value computed is %lu (expected 81)\n", result);
  printf("\n\n");

  delete eal;
  delete am;
}



int main ( void )
{
#if WTF_COMPILER_GCC
  test1();
  test2();
  test3();
#endif
  test4();
  test5();
  return 0;
}
