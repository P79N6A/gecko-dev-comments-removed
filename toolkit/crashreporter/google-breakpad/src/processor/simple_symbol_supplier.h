










































































#ifndef PROCESSOR_SIMPLE_SYMBOL_SUPPLIER_H__
#define PROCESSOR_SIMPLE_SYMBOL_SUPPLIER_H__

#include <map>
#include <string>
#include <vector>

#include "common/using_std_string.h"
#include "google_breakpad/processor/symbol_supplier.h"

namespace google_breakpad {

using std::map;
using std::vector;

class CodeModule;

class SimpleSymbolSupplier : public SymbolSupplier {
 public:
  
  
  explicit SimpleSymbolSupplier(const string &path) : paths_(1, path) {}

  
  
  explicit SimpleSymbolSupplier(const vector<string> &paths) : paths_(paths) {}

  virtual ~SimpleSymbolSupplier() {}

  
  
  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file);

  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file,
                                     string *symbol_data);

  
  
  virtual SymbolResult GetCStringSymbolData(const CodeModule *module,
                                            const SystemInfo *system_info,
                                            string *symbol_file,
                                            char **symbol_data);

  
  virtual void FreeSymbolData(const CodeModule *module);

 protected:
  SymbolResult GetSymbolFileAtPathFromRoot(const CodeModule *module,
                                           const SystemInfo *system_info,
                                           const string &root_path,
                                           string *symbol_file);

 private:
  map<string, char *> memory_buffers_;
  vector<string> paths_;
};

}  

#endif  
