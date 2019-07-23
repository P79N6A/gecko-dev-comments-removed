



































#include <cassert>

#include "google_breakpad/processor/stackwalker.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/symbol_supplier.h"
#include "processor/linked_ptr.h"
#include "processor/scoped_ptr.h"
#include "processor/stack_frame_info.h"
#include "processor/stackwalker_ppc.h"
#include "processor/stackwalker_x86.h"

namespace google_breakpad {


Stackwalker::Stackwalker(const SystemInfo *system_info,
                         MemoryRegion *memory,
                         const CodeModules *modules,
                         SymbolSupplier *supplier,
                         SourceLineResolverInterface *resolver)
    : system_info_(system_info),
      memory_(memory),
      modules_(modules),
      supplier_(supplier),
      resolver_(resolver) {
}


bool Stackwalker::Walk(CallStack *stack) {
  assert(stack);
  stack->Clear();

  
  
  
  vector< linked_ptr<StackFrameInfo> > stack_frame_info;

  
  

  
  scoped_ptr<StackFrame> frame(GetContextFrame());

  while (frame.get()) {
    
    
    

    linked_ptr<StackFrameInfo> frame_info;

    
    if (modules_) {
      const CodeModule *module =
          modules_->GetModuleForAddress(frame->instruction);
      if (module) {
        frame->module = module;
        if (resolver_ &&
            !resolver_->HasModule(frame->module->code_file()) &&
            supplier_) {
          string symbol_file;
          SymbolSupplier::SymbolResult symbol_result =
              supplier_->GetSymbolFile(module, system_info_, &symbol_file);

          switch (symbol_result) {
            case SymbolSupplier::FOUND:
              resolver_->LoadModule(frame->module->code_file(), symbol_file);
              break;
            case SymbolSupplier::NOT_FOUND:
              break;  
            case SymbolSupplier::INTERRUPT:
              return false;
          }
        }
        frame_info.reset(resolver_->FillSourceLineInfo(frame.get()));
      }
    }

    
    
    stack->frames_.push_back(frame.release());

    
    stack_frame_info.push_back(frame_info);
    frame_info.reset(NULL);

    
    frame.reset(GetCallerFrame(stack, stack_frame_info));
  }

  return true;
}



Stackwalker* Stackwalker::StackwalkerForCPU(
    const SystemInfo *system_info,
    MinidumpContext *context,
    MemoryRegion *memory,
    const CodeModules *modules,
    SymbolSupplier *supplier,
    SourceLineResolverInterface *resolver) {
  if (!context)
    return NULL;

  Stackwalker *cpu_stackwalker = NULL;

  u_int32_t cpu = context->GetContextCPU();
  switch (cpu) {
    case MD_CONTEXT_X86:
      cpu_stackwalker = new StackwalkerX86(system_info,
                                           context->GetContextX86(),
                                           memory, modules, supplier,
                                           resolver);
      break;

    case MD_CONTEXT_PPC:
      cpu_stackwalker = new StackwalkerPPC(system_info,
                                           context->GetContextPPC(),
                                           memory, modules, supplier,
                                           resolver);
      break;
  }

  return cpu_stackwalker;
}


}  
