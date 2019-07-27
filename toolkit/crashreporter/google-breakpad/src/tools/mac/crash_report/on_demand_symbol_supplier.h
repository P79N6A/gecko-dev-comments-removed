































#ifndef TOOLS_MAC_CRASH_REPORT_ON_DEMAND_SYMBOL_SUPPLIER_H__
#define TOOLS_MAC_CRASH_REPORT_ON_DEMAND_SYMBOL_SUPPLIER_H__

#include <map>
#include <string>
#include "google_breakpad/processor/symbol_supplier.h"

namespace google_breakpad {

using std::map;
using std::string;

class OnDemandSymbolSupplier : public SymbolSupplier {
 public:
  
  
  OnDemandSymbolSupplier(const string &search_dir,
                         const string &symbol_search_dir);
  virtual ~OnDemandSymbolSupplier() {}

  
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
  
  string search_dir_;
  string symbol_search_dir_;

  
  
  map<string, string> module_file_map_;

  
  map<string, char *> memory_buffers_;

  
  
  string GetNameForModule(const CodeModule *module);

  
  
  
  string GetLocalModulePath(const CodeModule *module);

  
  string GetModulePath(const CodeModule *module);

  
  
  string GetModuleSymbolFile(const CodeModule *module);

  
  
  bool GenerateSymbolFile(const CodeModule *module,
                          const SystemInfo *system_info);
};

}  

#endif  
