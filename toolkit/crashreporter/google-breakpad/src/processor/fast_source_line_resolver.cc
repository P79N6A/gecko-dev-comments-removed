






































#include "google_breakpad/processor/fast_source_line_resolver.h"
#include "processor/fast_source_line_resolver_types.h"

#include <map>
#include <string>
#include <utility>

#include "common/scoped_ptr.h"
#include "common/using_std_string.h"
#include "processor/module_factory.h"

using std::map;
using std::make_pair;

namespace google_breakpad {

FastSourceLineResolver::FastSourceLineResolver()
  : SourceLineResolverBase(new FastModuleFactory) { }

bool FastSourceLineResolver::ShouldDeleteMemoryBufferAfterLoadModule() {
  return false;
}

void FastSourceLineResolver::Module::LookupAddress(StackFrame *frame) const {
  MemAddr address = frame->instruction - frame->module->base_address();

  
  
  
  
  
  
  scoped_ptr<Function> func(new Function);
  const Function* func_ptr = 0;
  scoped_ptr<PublicSymbol> public_symbol(new PublicSymbol);
  const PublicSymbol* public_symbol_ptr = 0;
  MemAddr function_base;
  MemAddr function_size;
  MemAddr public_address;

  if (functions_.RetrieveNearestRange(address, func_ptr,
                                      &function_base, &function_size) &&
      address >= function_base && address - function_base < function_size) {
    func.get()->CopyFrom(func_ptr);
    frame->function_name = func->name;
    frame->function_base = frame->module->base_address() + function_base;

    scoped_ptr<Line> line(new Line);
    const Line* line_ptr = 0;
    MemAddr line_base;
    if (func->lines.RetrieveRange(address, line_ptr, &line_base, NULL)) {
      line.get()->CopyFrom(line_ptr);
      FileMap::iterator it = files_.find(line->source_file_id);
      if (it != files_.end()) {
        frame->source_file_name =
            files_.find(line->source_file_id).GetValuePtr();
      }
      frame->source_line = line->line;
      frame->source_line_base = frame->module->base_address() + line_base;
    }
  } else if (public_symbols_.Retrieve(address,
                                      public_symbol_ptr, &public_address) &&
             (!func_ptr || public_address > function_base)) {
    public_symbol.get()->CopyFrom(public_symbol_ptr);
    frame->function_name = public_symbol->name;
    frame->function_base = frame->module->base_address() + public_address;
  }
}



WindowsFrameInfo FastSourceLineResolver::CopyWFI(const char *raw) {
  const WindowsFrameInfo::StackInfoTypes type =
     static_cast<const WindowsFrameInfo::StackInfoTypes>(
         *reinterpret_cast<const int32_t*>(raw));

  
  
  
  const uint32_t *para_uint32 = reinterpret_cast<const uint32_t*>(
      raw + 2 * sizeof(int32_t));

  uint32_t prolog_size = para_uint32[0];;
  uint32_t epilog_size = para_uint32[1];
  uint32_t parameter_size = para_uint32[2];
  uint32_t saved_register_size = para_uint32[3];
  uint32_t local_size = para_uint32[4];
  uint32_t max_stack_size = para_uint32[5];
  const char *boolean = reinterpret_cast<const char*>(para_uint32 + 6);
  bool allocates_base_pointer = (*boolean != 0);
  string program_string = boolean + 1;

  return WindowsFrameInfo(type,
                          prolog_size,
                          epilog_size,
                          parameter_size,
                          saved_register_size,
                          local_size,
                          max_stack_size,
                          allocates_base_pointer,
                          program_string);
}




bool FastSourceLineResolver::Module::LoadMapFromMemory(char *mem_buffer) {
  if (!mem_buffer) return false;

  const uint32_t *map_sizes = reinterpret_cast<const uint32_t*>(mem_buffer);

  unsigned int header_size = kNumberMaps_ * sizeof(unsigned int);

  
  
  
  
  unsigned int offsets[kNumberMaps_];
  offsets[0] = header_size;
  for (int i = 1; i < kNumberMaps_; ++i) {
    offsets[i] = offsets[i - 1] + map_sizes[i - 1];
  }

  
  int map_id = 0;
  files_ = StaticMap<int, char>(mem_buffer + offsets[map_id++]);
  functions_ =
      StaticRangeMap<MemAddr, Function>(mem_buffer + offsets[map_id++]);
  public_symbols_ =
      StaticAddressMap<MemAddr, PublicSymbol>(mem_buffer + offsets[map_id++]);
  for (int i = 0; i < WindowsFrameInfo::STACK_INFO_LAST; ++i)
    windows_frame_info_[i] =
        StaticContainedRangeMap<MemAddr, char>(mem_buffer + offsets[map_id++]);

  cfi_initial_rules_ =
      StaticRangeMap<MemAddr, char>(mem_buffer + offsets[map_id++]);
  cfi_delta_rules_ = StaticMap<MemAddr, char>(mem_buffer + offsets[map_id++]);

  return true;
}

WindowsFrameInfo *FastSourceLineResolver::Module::FindWindowsFrameInfo(
    const StackFrame *frame) const {
  MemAddr address = frame->instruction - frame->module->base_address();
  scoped_ptr<WindowsFrameInfo> result(new WindowsFrameInfo());

  
  
  
  
  
  
  const char* frame_info_ptr;
  if ((windows_frame_info_[WindowsFrameInfo::STACK_INFO_FRAME_DATA]
       .RetrieveRange(address, frame_info_ptr))
      || (windows_frame_info_[WindowsFrameInfo::STACK_INFO_FPO]
          .RetrieveRange(address, frame_info_ptr))) {
    result->CopyFrom(CopyWFI(frame_info_ptr));
    return result.release();
  }

  
  
  
  
  
  
  
  scoped_ptr<Function> function(new Function);
  const Function* function_ptr = 0;
  MemAddr function_base, function_size;
  if (functions_.RetrieveNearestRange(address, function_ptr,
                                      &function_base, &function_size) &&
      address >= function_base && address - function_base < function_size) {
    function.get()->CopyFrom(function_ptr);
    result->parameter_size = function->parameter_size;
    result->valid |= WindowsFrameInfo::VALID_PARAMETER_SIZE;
    return result.release();
  }

  
  
  scoped_ptr<PublicSymbol> public_symbol(new PublicSymbol);
  const PublicSymbol* public_symbol_ptr = 0;
  MemAddr public_address;
  if (public_symbols_.Retrieve(address, public_symbol_ptr, &public_address) &&
      (!function_ptr || public_address > function_base)) {
    public_symbol.get()->CopyFrom(public_symbol_ptr);
    result->parameter_size = public_symbol->parameter_size;
  }

  return NULL;
}

CFIFrameInfo *FastSourceLineResolver::Module::FindCFIFrameInfo(
    const StackFrame *frame) const {
  MemAddr address = frame->instruction - frame->module->base_address();
  MemAddr initial_base, initial_size;
  const char* initial_rules = NULL;

  
  
  
  
  if (!cfi_initial_rules_.RetrieveRange(address, initial_rules,
                                        &initial_base, &initial_size)) {
    return NULL;
  }

  
  
  scoped_ptr<CFIFrameInfo> rules(new CFIFrameInfo());
  if (!ParseCFIRuleSet(initial_rules, rules.get()))
    return NULL;

  
  StaticMap<MemAddr, char>::iterator delta =
    cfi_delta_rules_.lower_bound(initial_base);

  
  while (delta != cfi_delta_rules_.end() && delta.GetKey() <= address) {
    ParseCFIRuleSet(delta.GetValuePtr(), rules.get());
    delta++;
  }

  return rules.release();
}

}  
