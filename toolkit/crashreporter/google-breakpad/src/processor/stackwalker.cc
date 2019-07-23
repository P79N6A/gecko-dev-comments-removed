



































#include "processor/stackwalker.h"
#include "google/call_stack.h"
#include "google/stack_frame.h"
#include "google/symbol_supplier.h"
#include "processor/linked_ptr.h"
#include "processor/minidump.h"
#include "processor/scoped_ptr.h"
#include "processor/source_line_resolver.h"
#include "processor/stack_frame_info.h"
#include "processor/stackwalker_ppc.h"
#include "processor/stackwalker_x86.h"

namespace google_airbag {


Stackwalker::Stackwalker(MemoryRegion *memory, MinidumpModuleList *modules,
                         SymbolSupplier *supplier)
    : memory_(memory), modules_(modules), supplier_(supplier) {
}


CallStack* Stackwalker::Walk() {
  SourceLineResolver resolver;

  scoped_ptr<CallStack> stack(new CallStack());

  
  
  
  vector< linked_ptr<StackFrameInfo> > stack_frame_info;

  
  

  
  scoped_ptr<StackFrame> frame(GetContextFrame());

  while (frame.get()) {
    
    
    

    linked_ptr<StackFrameInfo> frame_info;

    
    if (modules_) {
      MinidumpModule *module =
          modules_->GetModuleForAddress(frame->instruction);
      if (module) {
        frame->module_name = *(module->GetName());
        frame->module_base = module->base_address();
        if (!resolver.HasModule(frame->module_name) && supplier_) {
          string symbol_file = supplier_->GetSymbolFile(module);
          if (!symbol_file.empty()) {
            resolver.LoadModule(frame->module_name, symbol_file);
          }
        }
        frame_info.reset(resolver.FillSourceLineInfo(frame.get()));
      }
    }

    
    
    stack->frames_.push_back(frame.release());

    
    stack_frame_info.push_back(frame_info);
    frame_info.reset(NULL);

    
    frame.reset(GetCallerFrame(stack.get(), stack_frame_info));
  }

  return stack.release();
}



Stackwalker* Stackwalker::StackwalkerForCPU(MinidumpContext *context,
                                            MemoryRegion *memory,
                                            MinidumpModuleList *modules,
                                            SymbolSupplier *supplier) {
  Stackwalker *cpu_stackwalker = NULL;

  u_int32_t cpu = context->GetContextCPU();
  switch (cpu) {
    case MD_CONTEXT_X86:
      cpu_stackwalker = new StackwalkerX86(context->GetContextX86(),
                                           memory, modules, supplier);
      break;

    case MD_CONTEXT_PPC:
      cpu_stackwalker = new StackwalkerPPC(context->GetContextPPC(),
                                           memory, modules, supplier);
      break;
  }

  return cpu_stackwalker;
}


}  
