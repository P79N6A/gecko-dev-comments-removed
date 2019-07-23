



































#include <cassert>

#include "processor/basic_code_modules.h"
#include "google_breakpad/processor/code_module.h"
#include "processor/linked_ptr.h"
#include "processor/range_map-inl.h"

namespace google_breakpad {

BasicCodeModules::BasicCodeModules(const CodeModules *that)
    : main_address_(0),
      map_(new RangeMap<u_int64_t, linked_ptr<const CodeModule> >()) {
  assert(that);

  const CodeModule *main_module = that->GetMainModule();
  if (main_module)
    main_address_ = main_module->base_address();

  unsigned int count = that->module_count();
  for (unsigned int module_sequence = 0;
       module_sequence < count;
       ++module_sequence) {
    
    
    
    
    const CodeModule *module = that->GetModuleAtIndex(module_sequence)->Copy();
    map_->StoreRange(module->base_address(), module->size(),
                     linked_ptr<const CodeModule>(module));
  }
}

BasicCodeModules::~BasicCodeModules() {
  delete map_;
}

unsigned int BasicCodeModules::module_count() const {
  return map_->GetCount();
}

const CodeModule* BasicCodeModules::GetModuleForAddress(
    u_int64_t address) const {
  linked_ptr<const CodeModule> module;
  if (!map_->RetrieveRange(address, &module, NULL, NULL))
    return NULL;

  return module.get();
}

const CodeModule* BasicCodeModules::GetMainModule() const {
  return GetModuleForAddress(main_address_);
}

const CodeModule* BasicCodeModules::GetModuleAtSequence(
    unsigned int sequence) const {
  linked_ptr<const CodeModule> module;
  if (!map_->RetrieveRangeAtIndex(sequence, &module, NULL, NULL))
    return NULL;

  return module.get();
}

const CodeModule* BasicCodeModules::GetModuleAtIndex(
    unsigned int index) const {
  
  
  
  
  return GetModuleAtSequence(index);
}

const CodeModules* BasicCodeModules::Copy() const {
  return new BasicCodeModules(this);
}

}  
