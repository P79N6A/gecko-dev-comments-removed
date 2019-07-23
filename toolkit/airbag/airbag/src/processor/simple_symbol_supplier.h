


































































#ifndef PROCESSOR_SIMPLE_SYMBOL_SUPPLIER_H__
#define PROCESSOR_SIMPLE_SYMBOL_SUPPLIER_H__

#include <string>

#include "google_airbag/processor/symbol_supplier.h"

namespace google_airbag {

using std::string;

class MinidumpModule;

class SimpleSymbolSupplier : public SymbolSupplier {
 public:
  
  
  explicit SimpleSymbolSupplier(const string &path) : path_(path) {}

  virtual ~SimpleSymbolSupplier() {}

  
  
  virtual string GetSymbolFile(MinidumpModule *module) {
    return GetSymbolFileAtPath(module, path_);
  }

 protected:
  string GetSymbolFileAtPath(MinidumpModule *module, const string &root_path);

 private:
  string path_;
};

}  

#endif  
