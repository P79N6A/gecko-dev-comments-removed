

































#include "google_breakpad/processor/stack_frame_symbolizer.h"

#include <assert.h>

#include "common/scoped_ptr.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/symbol_supplier.h"
#include "google_breakpad/processor/system_info.h"
#include "processor/linked_ptr.h"
#include "processor/logging.h"

namespace google_breakpad {

StackFrameSymbolizer::StackFrameSymbolizer(
    SymbolSupplier* supplier,
    SourceLineResolverInterface* resolver) : supplier_(supplier),
                                             resolver_(resolver) { }

StackFrameSymbolizer::SymbolizerResult StackFrameSymbolizer::FillSourceLineInfo(
    const CodeModules* modules,
    const SystemInfo* system_info,
    StackFrame* frame) {
  assert(frame);

  if (!modules) return ERROR;
  const CodeModule* module = modules->GetModuleForAddress(frame->instruction);
  if (!module) return ERROR;
  frame->module = module;

  if (!resolver_) return ERROR;  
  
  if (no_symbol_modules_.find(module->code_file()) !=
      no_symbol_modules_.end()) {
    return ERROR;
  }

  
  if (resolver_->HasModule(frame->module)) {
    resolver_->FillSourceLineInfo(frame);
    return NO_ERROR;
  }

  
  if (!supplier_) {
    return ERROR;
  }

  
  string symbol_file;
  char* symbol_data = NULL;
  SymbolSupplier::SymbolResult symbol_result = supplier_->GetCStringSymbolData(
      module, system_info, &symbol_file, &symbol_data);

  switch (symbol_result) {
    case SymbolSupplier::FOUND: {
      bool load_success = resolver_->LoadModuleUsingMemoryBuffer(frame->module,
                                                                 symbol_data);
      if (resolver_->ShouldDeleteMemoryBufferAfterLoadModule()) {
        supplier_->FreeSymbolData(module);
      }

      if (load_success) {
        resolver_->FillSourceLineInfo(frame);
        return NO_ERROR;
      } else {
        BPLOG(ERROR) << "Failed to load symbol file in resolver.";
        no_symbol_modules_.insert(module->code_file());
        return ERROR;
      }
    }

    case SymbolSupplier::NOT_FOUND:
      no_symbol_modules_.insert(module->code_file());
      return ERROR;

    case SymbolSupplier::INTERRUPT:
      return INTERRUPT;

    default:
      BPLOG(ERROR) << "Unknown SymbolResult enum: " << symbol_result;
      return ERROR;
  }
  return ERROR;
}

WindowsFrameInfo* StackFrameSymbolizer::FindWindowsFrameInfo(
    const StackFrame* frame) {
  return resolver_ ? resolver_->FindWindowsFrameInfo(frame) : NULL;
}

CFIFrameInfo* StackFrameSymbolizer::FindCFIFrameInfo(
    const StackFrame* frame) {
  return resolver_ ? resolver_->FindCFIFrameInfo(frame) : NULL;
}

}  
