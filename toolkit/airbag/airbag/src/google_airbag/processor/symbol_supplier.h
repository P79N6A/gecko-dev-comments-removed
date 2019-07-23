































#ifndef GOOGLE_AIRBAG_PROCESSOR_SYMBOL_SUPPLIER_H__
#define GOOGLE_AIRBAG_PROCESSOR_SYMBOL_SUPPLIER_H__

#include <string>

namespace google_airbag {

using std::string;
class MinidumpModule;

class SymbolSupplier {
 public:
  virtual ~SymbolSupplier() {}

  
  virtual string GetSymbolFile(MinidumpModule *module) = 0;
};

}  

#endif  
