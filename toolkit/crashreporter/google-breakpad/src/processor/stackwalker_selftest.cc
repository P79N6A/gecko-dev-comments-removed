


















































#include "processor/logging.h"

#if defined(__i386) && !defined(__i386__)
#define __i386__
#endif
#if defined(__sparc) && !defined(__sparc__)
#define __sparc__
#endif
 
#if (defined(__SUNPRO_CC) || defined(__GNUC__)) && \
    (defined(__i386__) || defined(__ppc__) || defined(__sparc__))


#include <stdio.h>

#include "common/scoped_ptr.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/stack_frame_cpu.h"

using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::CodeModule;
using google_breakpad::MemoryRegion;
using google_breakpad::scoped_ptr;
using google_breakpad::StackFrame;
using google_breakpad::StackFramePPC;
using google_breakpad::StackFrameX86;
using google_breakpad::StackFrameSPARC;

#if defined(__i386__)
#include "processor/stackwalker_x86.h"
using google_breakpad::StackwalkerX86;
#elif defined(__ppc__)
#include "processor/stackwalker_ppc.h"
using google_breakpad::StackwalkerPPC;
#elif defined(__sparc__)
#include "processor/stackwalker_sparc.h"
using google_breakpad::StackwalkerSPARC;
#endif  

#define RECURSION_DEPTH 100




class SelfMemoryRegion : public MemoryRegion {
 public:
  virtual uint64_t GetBase() { return 0; }
  virtual uint32_t GetSize() { return 0xffffffff; }

  bool GetMemoryAtAddress(uint64_t address, uint8_t*  value) {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(uint64_t address, uint16_t* value) {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(uint64_t address, uint32_t* value) {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(uint64_t address, uint64_t* value) {
      return GetMemoryAtAddressInternal(address, value); }

 private:
  template<typename T> bool GetMemoryAtAddressInternal(uint64_t address,
                                                       T*        value) {
    
    
    
    
    
    if (address < 0x100)
      return false;

    uint8_t* memory = 0;
    *value = *reinterpret_cast<const T*>(&memory[address]);
    return true;
  }
};


#if defined(__GNUC__)


#if defined(__i386__)








static uint32_t GetEBP() __attribute__((noinline));
static uint32_t GetEBP() {
  uint32_t ebp;
  __asm__ __volatile__(
    "movl (%%ebp), %0"
    : "=a" (ebp)
  );
  return ebp;
}







static uint32_t GetESP() __attribute__((noinline));
static uint32_t GetESP() {
  uint32_t ebp;
  __asm__ __volatile__(
    "movl %%ebp, %0"
    : "=a" (ebp)
  );
  return ebp + 8;
}












static uint32_t GetEIP() __attribute__((noinline));
static uint32_t GetEIP() {
  uint32_t eip;
  __asm__ __volatile__(
    "movl 4(%%ebp), %0"
    : "=a" (eip)
  );
  return eip;
}


#elif defined(__ppc__)








static uint32_t GetSP() __attribute__((noinline));
static uint32_t GetSP() {
  uint32_t sp;
  __asm__ __volatile__(
    "lwz %0, 0(r1)"
    : "=r" (sp)
  );
  return sp;
}







static uint32_t GetPC() __attribute__((noinline));
static uint32_t GetPC() {
  uint32_t lr;
  __asm__ __volatile__(
    "mflr %0"
    : "=r" (lr)
  );
  return lr;
}


#elif defined(__sparc__)









static uint32_t GetSP() __attribute__((noinline));
static uint32_t GetSP() {
  uint32_t sp;
  __asm__ __volatile__(
    "mov %%fp, %0"
    : "=r" (sp)
  );
  return sp;
}








static uint32_t GetFP() __attribute__((noinline));
static uint32_t GetFP() {
  uint32_t fp;
  __asm__ __volatile__(
    "ld [%%fp+56], %0"
    : "=r" (fp)
  );
  return fp;
}






static uint32_t GetPC() __attribute__((noinline));
static uint32_t GetPC() {
  uint32_t pc;
  __asm__ __volatile__(
    "mov %%i7, %0"
    : "=r" (pc)
  );
  return pc + 8;
}

#endif  

#elif defined(__SUNPRO_CC)

#if defined(__i386__)
extern "C" {
extern uint32_t GetEIP();
extern uint32_t GetEBP();
extern uint32_t GetESP();
}
#elif defined(__sparc__)
extern "C" {
extern uint32_t GetPC();
extern uint32_t GetFP();
extern uint32_t GetSP();
}
#endif 

#endif 





#if defined(__GNUC__)
static unsigned int CountCallerFrames() __attribute__((noinline));
#elif defined(__SUNPRO_CC)
static unsigned int CountCallerFrames();
#endif
static unsigned int CountCallerFrames() {
  SelfMemoryRegion memory;
  BasicSourceLineResolver resolver;

#if defined(__i386__)
  MDRawContextX86 context = MDRawContextX86();
  context.eip = GetEIP();
  context.ebp = GetEBP();
  context.esp = GetESP();

  StackwalkerX86 stackwalker = StackwalkerX86(NULL, &context, &memory, NULL,
                                              NULL, &resolver);
#elif defined(__ppc__)
  MDRawContextPPC context = MDRawContextPPC();
  context.srr0 = GetPC();
  context.gpr[1] = GetSP();

  StackwalkerPPC stackwalker = StackwalkerPPC(NULL, &context, &memory, NULL,
                                              NULL, &resolver);
#elif defined(__sparc__)
  MDRawContextSPARC context = MDRawContextSPARC();
  context.pc = GetPC();
  context.g_r[14] = GetSP();
  context.g_r[30] = GetFP();

  StackwalkerSPARC stackwalker = StackwalkerSPARC(NULL, &context, &memory,
                                                  NULL, NULL, &resolver);
#endif  

  CallStack stack;
  vector<const CodeModule*> modules_without_symbols;
  stackwalker.Walk(&stack, &modules_without_symbols);

#ifdef PRINT_STACKS
  printf("\n");
  for (unsigned int frame_index = 0;
      frame_index < stack.frames()->size();
      ++frame_index) {
    StackFrame *frame = stack.frames()->at(frame_index);
    printf("frame %-3d  instruction = 0x%08" PRIx64,
           frame_index, frame->instruction);
#if defined(__i386__)
    StackFrameX86 *frame_x86 = reinterpret_cast<StackFrameX86*>(frame);
    printf("  esp = 0x%08x  ebp = 0x%08x\n",
           frame_x86->context.esp, frame_x86->context.ebp);
#elif defined(__ppc__)
    StackFramePPC *frame_ppc = reinterpret_cast<StackFramePPC*>(frame);
    printf("  gpr[1] = 0x%08x\n", frame_ppc->context.gpr[1]);
#elif defined(__sparc__)
    StackFrameSPARC *frame_sparc = reinterpret_cast<StackFrameSPARC*>(frame);
    printf("  sp = 0x%08x  fp = 0x%08x\n",
           frame_sparc->context.g_r[14], frame_sparc->context.g_r[30]);
#endif  
  }
#endif  

  
  
  
  return stack.frames()->size() - 2;
}







#if defined(__GNUC__)
static bool Recursor(unsigned int depth, unsigned int parent_callers)
    __attribute__((noinline));
#elif defined(__SUNPRO_CC)
static bool Recursor(unsigned int depth, unsigned int parent_callers);
#endif
static bool Recursor(unsigned int depth, unsigned int parent_callers) {
  unsigned int callers = CountCallerFrames();
  if (callers != parent_callers + 1)
    return false;

  if (depth)
    return Recursor(depth - 1, callers);

  
  return true;
}





#if defined(__GNUC__)
int main(int argc, char** argv) __attribute__((noinline));
#elif defined(__SUNPRO_CC)
int main(int argc, char** argv);
#endif
int main(int argc, char** argv) {
  BPLOG_INIT(&argc, &argv);

  return Recursor(RECURSION_DEPTH, CountCallerFrames()) ? 0 : 1;
}


#else



int main(int argc, char **argv) {
  BPLOG_INIT(&argc, &argv);

  
  
  BPLOG(ERROR) << "Selftest not supported here";
  return 77;
}


#endif  
