


































#include "processor/module_serializer.h"

#include <map>
#include <string>

#include "processor/basic_code_module.h"
#include "processor/logging.h"

namespace google_breakpad {



RangeMapSerializer< MemAddr, linked_ptr<BasicSourceLineResolver::Line> >
SimpleSerializer<BasicSourceLineResolver::Function>::range_map_serializer_;

size_t ModuleSerializer::SizeOf(const BasicSourceLineResolver::Module &module) {
  size_t total_size_alloc_ = 0;

  
  int map_index = 0;
  map_sizes_[map_index++] = files_serializer_.SizeOf(module.files_);
  map_sizes_[map_index++] = functions_serializer_.SizeOf(module.functions_);
  map_sizes_[map_index++] = pubsym_serializer_.SizeOf(module.public_symbols_);
  for (int i = 0; i < WindowsFrameInfo::STACK_INFO_LAST; ++i)
   map_sizes_[map_index++] =
       wfi_serializer_.SizeOf(&(module.windows_frame_info_[i]));
  map_sizes_[map_index++] = cfi_init_rules_serializer_.SizeOf(
     module.cfi_initial_rules_);
  map_sizes_[map_index++] = cfi_delta_rules_serializer_.SizeOf(
     module.cfi_delta_rules_);

  
  total_size_alloc_ = kNumberMaps_ * sizeof(u_int32_t);

  for (int i = 0; i < kNumberMaps_; ++i)
   total_size_alloc_ += map_sizes_[i];

  
  ++total_size_alloc_;

  return total_size_alloc_;
}

char *ModuleSerializer::Write(const BasicSourceLineResolver::Module &module,
                              char *dest) {
  
  memcpy(dest, map_sizes_, kNumberMaps_ * sizeof(u_int32_t));
  dest += kNumberMaps_ * sizeof(u_int32_t);
  
  dest = files_serializer_.Write(module.files_, dest);
  dest = functions_serializer_.Write(module.functions_, dest);
  dest = pubsym_serializer_.Write(module.public_symbols_, dest);
  for (int i = 0; i < WindowsFrameInfo::STACK_INFO_LAST; ++i)
    dest = wfi_serializer_.Write(&(module.windows_frame_info_[i]), dest);
  dest = cfi_init_rules_serializer_.Write(module.cfi_initial_rules_, dest);
  dest = cfi_delta_rules_serializer_.Write(module.cfi_delta_rules_, dest);
  
  dest = SimpleSerializer<char>::Write(0, dest);
  return dest;
}

char* ModuleSerializer::Serialize(
    const BasicSourceLineResolver::Module &module, unsigned int *size) {
  
  unsigned int size_to_alloc = SizeOf(module);

  
  char *serialized_data = new char[size_to_alloc];
  if (!serialized_data) {
    BPLOG(ERROR) << "ModuleSerializer: memory allocation failed, "
                 << "size to alloc: " << size_to_alloc;
    if (size) *size = 0;
    return NULL;
  }

  
  char *end_address = Write(module, serialized_data);
  
  unsigned int size_written =
      static_cast<unsigned int>(end_address - serialized_data);
  if (size_to_alloc != size_written) {
    BPLOG(ERROR) << "size_to_alloc differs from size_written: "
                   << size_to_alloc << " vs " << size_written;
  }

  
  if (size)
    *size = size_to_alloc;
  return serialized_data;
}

bool ModuleSerializer::SerializeModuleAndLoadIntoFastResolver(
    const BasicSourceLineResolver::ModuleMap::const_iterator &iter,
    FastSourceLineResolver *fast_resolver) {
  BPLOG(INFO) << "Converting symbol " << iter->first.c_str();

  
  BasicSourceLineResolver::Module* basic_module =
      dynamic_cast<BasicSourceLineResolver::Module*>(iter->second);

  unsigned int size = 0;
  scoped_array<char> symbol_data(Serialize(*basic_module, &size));
  if (!symbol_data.get()) {
    BPLOG(ERROR) << "Serialization failed for module: " << basic_module->name_;
    return false;
  }
  BPLOG(INFO) << "Serialized Symbol Size " << size;

  
  
  
  string symbol_data_string(symbol_data.get(), size);
  symbol_data.reset();

  scoped_ptr<CodeModule> code_module(
      new BasicCodeModule(0, 0, iter->first, "", "", "", ""));

  return fast_resolver->LoadModuleUsingMapBuffer(code_module.get(),
                                                 symbol_data_string);
}

void ModuleSerializer::ConvertAllModules(
    const BasicSourceLineResolver *basic_resolver,
    FastSourceLineResolver *fast_resolver) {
  
  if (!basic_resolver || !fast_resolver)
    return;

  
  BasicSourceLineResolver::ModuleMap::const_iterator iter;
  iter = basic_resolver->modules_->begin();
  for (; iter != basic_resolver->modules_->end(); ++iter)
    SerializeModuleAndLoadIntoFastResolver(iter, fast_resolver);
}

bool ModuleSerializer::ConvertOneModule(
    const string &moduleid,
    const BasicSourceLineResolver *basic_resolver,
    FastSourceLineResolver *fast_resolver) {
  
  if (!basic_resolver || !fast_resolver)
    return false;

  BasicSourceLineResolver::ModuleMap::const_iterator iter;
  iter = basic_resolver->modules_->find(moduleid);
  if (iter == basic_resolver->modules_->end())
    return false;

  return SerializeModuleAndLoadIntoFastResolver(iter, fast_resolver);
}

char* ModuleSerializer::SerializeSymbolFileData(
    const string &symbol_data, unsigned int *size) {
  scoped_ptr<BasicSourceLineResolver::Module> module(
      new BasicSourceLineResolver::Module("no name"));
  scoped_array<char> buffer(new char[symbol_data.size() + 1]);
  strcpy(buffer.get(), symbol_data.c_str());
  if (!module->LoadMapFromMemory(buffer.get())) {
    return NULL;
  }
  buffer.reset(NULL);
  return Serialize(*(module.get()), size);
}

}  
