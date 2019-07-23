































#ifndef GOOGLE_BREAKPAD_PROCESSOR_SYMBOL_SUPPLIER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SYMBOL_SUPPLIER_H__

#include <string>

namespace google_breakpad {

using std::string;
class CodeModule;
class SystemInfo;

class SymbolSupplier {
 public:
  
  enum SymbolResult {
    
    NOT_FOUND,

    
    FOUND,

    
    INTERRUPT
  };

  virtual ~SymbolSupplier() {}

  
  
  
  
  
  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file) = 0;
};

}  

#endif
