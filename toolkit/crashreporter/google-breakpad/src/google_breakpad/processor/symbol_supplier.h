































#ifndef GOOGLE_BREAKPAD_PROCESSOR_SYMBOL_SUPPLIER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SYMBOL_SUPPLIER_H__

#include <string>
#include "common/using_std_string.h"

namespace google_breakpad {

class CodeModule;
struct SystemInfo;

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
  
  
  
  
  
  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file,
                                     string *symbol_data) = 0;

  
  
  
  
  
  
  
  
  virtual SymbolResult GetCStringSymbolData(const CodeModule *module,
                                            const SystemInfo *system_info,
                                            string *symbol_file,
                                            char **symbol_data) = 0;

  
  virtual void FreeSymbolData(const CodeModule *module) = 0;
};

}  

#endif
