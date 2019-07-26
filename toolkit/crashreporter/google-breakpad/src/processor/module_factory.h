


































#ifndef PROCESSOR_MODULE_FACTORY_H__
#define PROCESSOR_MODULE_FACTORY_H__

#include "processor/basic_source_line_resolver_types.h"
#include "processor/fast_source_line_resolver_types.h"
#include "processor/source_line_resolver_base_types.h"

namespace google_breakpad {

class ModuleFactory {
 public:
  virtual ~ModuleFactory() { };
  virtual SourceLineResolverBase::Module* CreateModule(
      const string &name) const = 0;
};

class BasicModuleFactory : public ModuleFactory {
 public:
  virtual ~BasicModuleFactory() { }
  virtual BasicSourceLineResolver::Module* CreateModule(
      const string &name) const {
    return new BasicSourceLineResolver::Module(name);
  }
};

class FastModuleFactory : public ModuleFactory {
 public:
  virtual ~FastModuleFactory() { }
  virtual FastSourceLineResolver::Module* CreateModule(
      const string &name) const {
    return new FastSourceLineResolver::Module(name);
  }
};

}  

#endif  
