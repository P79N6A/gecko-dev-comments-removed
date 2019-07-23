


















































#if defined(__GNUC__) && (defined(__i386__) || defined(__ppc__))


#include <cstdio>

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/logging.h"
#include "processor/scoped_ptr.h"

using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::MemoryRegion;
using google_breakpad::scoped_ptr;
using google_breakpad::StackFrame;
using google_breakpad::StackFramePPC;
using google_breakpad::StackFrameX86;

#if defined(__i386__)
#include "processor/stackwalker_x86.h"
using google_breakpad::StackwalkerX86;
#elif defined(__ppc__)
#include "processor/stackwalker_ppc.h"
using google_breakpad::StackwalkerPPC;
#endif  

#define RECURSION_DEPTH 100




class SelfMemoryRegion : public MemoryRegion {
 public:
  virtual u_int64_t GetBase() { return 0; }
  virtual u_int32_t GetSize() { return 0xffffffff; }

  bool GetMemoryAtAddress(u_int64_t address, u_int8_t*  value) {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(u_int64_t address, u_int16_t* value) {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(u_int64_t address, u_int32_t* value) {
      return GetMemoryAtAddressInternal(address, value); }
  bool GetMemoryAtAddress(u_int64_t address, u_int64_t* value) {
      return GetMemoryAtAddressInternal(address, value); }

 private:
  template<typename T> bool GetMemoryAtAddressInternal(u_int64_t address,
                                                       T*        value) {
    
    
    
    
    
    if (address < 0x100)
      return false;

    u_int8_t* memory = 0;
    *value = *reinterpret_cast<const T*>(&memory[address]);
    return true;
  }
};


#if defined(__i386__)








static u_int32_t GetEBP() __attribute__((noinline));
static u_int32_t GetEBP() {
  u_int32_t ebp;
  __asm__ __volatile__(
    "movl (%%ebp), %0"
    : "=a" (ebp)
  );
  return ebp;
}







static u_int32_t GetESP() __attribute__((noinline));
static u_int32_t GetESP() {
  u_int32_t ebp;
  __asm__ __volatile__(
    "movl %%ebp, %0"
    : "=a" (ebp)
  );
  return ebp + 8;
}












static u_int32_t GetEIP() __attribute__((noinline));
static u_int32_t GetEIP() {
  u_int32_t eip;
  __asm__ __volatile__(
    "movl 4(%%ebp), %0"
    : "=a" (eip)
  );
  return eip;
}


#elif defined(__ppc__)








static u_int32_t GetSP() __attribute__((noinline));
static u_int32_t GetSP() {
  u_int32_t sp;
  __asm__ __volatile__(
    "lwz %0, 0(r1)"
    : "=r" (sp)
  );
  return sp;
}







static u_int32_t GetPC() __attribute__((noinline));
static u_int32_t GetPC() {
  u_int32_t lr;
  __asm__ __volatile__(
    "mflr %0"
    : "=r" (lr)
  );
  return lr;
}


#endif  






static unsigned int CountCallerFrames() __attribute__((noinline));
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
#endif  

  CallStack stack;
  stackwalker.Walk(&stack);

#ifdef PRINT_STACKS
  printf("\n");
  for (unsigned int frame_index = 0;
      frame_index < stack.frames()->size();
      ++frame_index) {
    StackFrame *frame = stack.frames()->at(frame_index);
    printf("frame %-3d  instruction = 0x%08llx",
           frame_index, frame->instruction);
#if defined(__i386__)
    StackFrameX86 *frame_x86 = reinterpret_cast<StackFrameX86*>(frame);
    printf("  esp = 0x%08x  ebp = 0x%08x\n",
           frame_x86->context.esp, frame_x86->context.ebp);
#elif defined(__ppc__)
    StackFramePPC *frame_ppc = reinterpret_cast<StackFramePPC*>(frame);
    printf("  gpr[1] = 0x%08x\n", frame_ppc->context.gpr[1]);
#endif  
  }
#endif  

  
  
  
  return stack.frames()->size() - 2;
}







static bool Recursor(unsigned int depth, unsigned int parent_callers)
    __attribute__((noinline));
static bool Recursor(unsigned int depth, unsigned int parent_callers) {
  unsigned int callers = CountCallerFrames();
  if (callers != parent_callers + 1)
    return false;

  if (depth)
    return Recursor(depth - 1, callers);

  
  return true;
}





int main(int argc, char** argv) __attribute__((noinline));
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
