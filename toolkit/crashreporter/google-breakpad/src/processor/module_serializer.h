



































#ifndef PROCESSOR_MODULE_SERIALIZER_H__
#define PROCESSOR_MODULE_SERIALIZER_H__

#include <map>
#include <string>

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/fast_source_line_resolver.h"
#include "processor/basic_source_line_resolver_types.h"
#include "processor/fast_source_line_resolver_types.h"
#include "processor/linked_ptr.h"
#include "processor/map_serializers-inl.h"
#include "processor/simple_serializer-inl.h"
#include "processor/windows_frame_info.h"

namespace google_breakpad {







class ModuleSerializer {
 public:
  
  
  size_t SizeOf(const BasicSourceLineResolver::Module &module);

  
  
  char* Write(const BasicSourceLineResolver::Module &module, char *dest);

  
  
  
  
  
  char* Serialize(const BasicSourceLineResolver::Module &module,
                  unsigned int *size = NULL);

  
  
  
  char* SerializeSymbolFileData(const string &symbol_data,
                                unsigned int *size = NULL);

  
  
  
  
  bool ConvertOneModule(const string &moduleid,
                        const BasicSourceLineResolver *basic_resolver,
                        FastSourceLineResolver *fast_resolver);

  
  
  void ConvertAllModules(const BasicSourceLineResolver *basic_resolver,
                         FastSourceLineResolver *fast_resolver);

 private:
  
  typedef BasicSourceLineResolver::Line Line;
  typedef BasicSourceLineResolver::Function Function;
  typedef BasicSourceLineResolver::PublicSymbol PublicSymbol;

  
  bool SerializeModuleAndLoadIntoFastResolver(
      const BasicSourceLineResolver::ModuleMap::const_iterator &iter,
      FastSourceLineResolver *fast_resolver);

  
  static const int32_t kNumberMaps_ =
      FastSourceLineResolver::Module::kNumberMaps_;

  
  uint32_t map_sizes_[kNumberMaps_];

  
  StdMapSerializer<int, string> files_serializer_;
  RangeMapSerializer<MemAddr, linked_ptr<Function> > functions_serializer_;
  AddressMapSerializer<MemAddr, linked_ptr<PublicSymbol> > pubsym_serializer_;
  ContainedRangeMapSerializer<MemAddr,
                              linked_ptr<WindowsFrameInfo> > wfi_serializer_;
  RangeMapSerializer<MemAddr, string> cfi_init_rules_serializer_;
  StdMapSerializer<MemAddr, string> cfi_delta_rules_serializer_;
};

}  

#endif  
