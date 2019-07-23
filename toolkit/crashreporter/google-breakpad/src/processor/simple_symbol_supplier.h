










































































#ifndef PROCESSOR_SIMPLE_SYMBOL_SUPPLIER_H__
#define PROCESSOR_SIMPLE_SYMBOL_SUPPLIER_H__

#include <string>
#include <vector>

#include "google_breakpad/processor/symbol_supplier.h"

namespace google_breakpad {

using std::string;
using std::vector;

class CodeModule;

class SimpleSymbolSupplier : public SymbolSupplier {
 public:
  
  
  explicit SimpleSymbolSupplier(const string &path) : paths_(1, path) {}

  
  
  explicit SimpleSymbolSupplier(const vector<string> &paths) : paths_(paths) {}

  virtual ~SimpleSymbolSupplier() {}

  
  
  SymbolResult GetSymbolFile(const CodeModule *module,
                             const SystemInfo *system_info,
                             string *symbol_file);

 protected:
  SymbolResult GetSymbolFileAtPath(const CodeModule *module,
                                   const SystemInfo *system_info,
                                   const string &root_path,
                                   string *symbol_file);

 private:
  vector<string> paths_;
};

}  

#endif  
