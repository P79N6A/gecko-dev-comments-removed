







































#ifndef PROCESSOR_BASIC_CODE_MODULES_H__
#define PROCESSOR_BASIC_CODE_MODULES_H__

#include "google_breakpad/processor/code_modules.h"

namespace google_breakpad {

template<typename T> class linked_ptr;
template<typename AddressType, typename EntryType> class RangeMap;

class BasicCodeModules : public CodeModules {
 public:
  
  
  
  
  
  explicit BasicCodeModules(const CodeModules *that);

  virtual ~BasicCodeModules();

  
  virtual unsigned int module_count() const;
  virtual const CodeModule* GetModuleForAddress(u_int64_t address) const;
  virtual const CodeModule* GetMainModule() const;
  virtual const CodeModule* GetModuleAtSequence(unsigned int sequence) const;
  virtual const CodeModule* GetModuleAtIndex(unsigned int index) const;
  virtual const CodeModules* Copy() const;

 private:
  
  u_int64_t main_address_;

  
  
  RangeMap<u_int64_t, linked_ptr<const CodeModule> > *map_;

  
  BasicCodeModules(const BasicCodeModules &that);
  void operator=(const BasicCodeModules &that);
};

}  

#endif  
