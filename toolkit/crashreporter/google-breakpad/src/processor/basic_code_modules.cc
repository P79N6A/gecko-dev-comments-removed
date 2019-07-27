



































#include "processor/basic_code_modules.h"

#include <assert.h>

#include "google_breakpad/processor/code_module.h"
#include "processor/linked_ptr.h"
#include "common/logging.h"
#include "processor/range_map-inl.h"

namespace google_breakpad {

BasicCodeModules::BasicCodeModules(const CodeModules *that)
    : main_address_(0),
      map_(new RangeMap<uint64_t, linked_ptr<const CodeModule> >()) {
  BPLOG_IF(ERROR, !that) << "BasicCodeModules::BasicCodeModules requires "
                            "|that|";
  assert(that);

  const CodeModule *main_module = that->GetMainModule();
  if (main_module)
    main_address_ = main_module->base_address();

  unsigned int count = that->module_count();
  for (unsigned int module_sequence = 0;
       module_sequence < count;
       ++module_sequence) {
    
    
    
    
    linked_ptr<const CodeModule> module(
        that->GetModuleAtIndex(module_sequence)->Copy());
    if (!map_->StoreRange(module->base_address(), module->size(), module)) {
      BPLOG(ERROR) << "Module " << module->code_file() <<
                      " could not be stored";
    }
  }
}

BasicCodeModules::~BasicCodeModules() {
  delete map_;
}

unsigned int BasicCodeModules::module_count() const {
  return map_->GetCount();
}

const CodeModule* BasicCodeModules::GetModuleForAddress(
    uint64_t address) const {
  linked_ptr<const CodeModule> module;
  if (!map_->RetrieveRange(address, &module, NULL, NULL)) {
    BPLOG(INFO) << "No module at " << HexString(address);
    return NULL;
  }

  return module.get();
}

const CodeModule* BasicCodeModules::GetMainModule() const {
  return GetModuleForAddress(main_address_);
}

const CodeModule* BasicCodeModules::GetModuleAtSequence(
    unsigned int sequence) const {
  linked_ptr<const CodeModule> module;
  if (!map_->RetrieveRangeAtIndex(sequence, &module, NULL, NULL)) {
    BPLOG(ERROR) << "RetrieveRangeAtIndex failed for sequence " << sequence;
    return NULL;
  }

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
