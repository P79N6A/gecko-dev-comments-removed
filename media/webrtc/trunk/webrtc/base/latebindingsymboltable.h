









#ifndef WEBRTC_BASE_LATEBINDINGSYMBOLTABLE_H_
#define WEBRTC_BASE_LATEBINDINGSYMBOLTABLE_H_

#include <string.h>

#include "webrtc/base/common.h"

namespace rtc {

#if defined(WEBRTC_POSIX)
typedef void *DllHandle;
#else
#error Not implemented for this platform
#endif







class LateBindingSymbolTable {
 public:
  struct TableInfo {
    const char *dll_name;
    int num_symbols;
    
    const char *const *symbol_names;
  };

  LateBindingSymbolTable(const TableInfo *info, void **table);
  ~LateBindingSymbolTable();

  bool IsLoaded() const;
  
  
  bool Load();
  
  
  bool LoadFromPath(const char *dll_path);
  void Unload();

  
  DllHandle GetDllHandle() const { return handle_; }

 private:
  void ClearSymbols();

  const TableInfo *info_;
  void **table_;
  DllHandle handle_;
  bool undefined_symbols_;

  DISALLOW_COPY_AND_ASSIGN(LateBindingSymbolTable);
};

}  

#endif  
