


























#ifndef WEBRTC_AUDIO_DEVICE_LATEBINDINGSYMBOLTABLE_LINUX_H
#define WEBRTC_AUDIO_DEVICE_LATEBINDINGSYMBOLTABLE_LINUX_H

#include <assert.h>
#include <stddef.h>  
#include <string.h>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/trace.h"






namespace webrtc_adm_linux {

#if defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
typedef void *DllHandle;

const DllHandle kInvalidDllHandle = NULL;
#else
#error Not implemented
#endif


DllHandle InternalLoadDll(const char dll_name[]);

void InternalUnloadDll(DllHandle handle);

bool InternalLoadSymbols(DllHandle handle,
                         int num_symbols,
                         const char *const symbol_names[],
                         void *symbols[]);

template <int SYMBOL_TABLE_SIZE,
          const char kDllName[],
          const char *const kSymbolNames[]>
class LateBindingSymbolTable {
 public:
  LateBindingSymbolTable()
      : handle_(kInvalidDllHandle),
        undefined_symbols_(false) {
    memset(symbols_, 0, sizeof(symbols_));
  }

  ~LateBindingSymbolTable() {
    Unload();
  }

  static int NumSymbols() {
    return SYMBOL_TABLE_SIZE;
  }

  
  static const char *GetSymbolName(int index) {
    assert(index < NumSymbols());
    return kSymbolNames[index];
  }

  bool IsLoaded() const {
    return handle_ != kInvalidDllHandle;
  }

  
  
  bool Load() {
    if (IsLoaded()) {
      return true;
    }
    if (undefined_symbols_) {
      
      
      
      
      return false;
    }
    handle_ = InternalLoadDll(kDllName);
    if (!IsLoaded()) {
      return false;
    }
    if (!InternalLoadSymbols(handle_, NumSymbols(), kSymbolNames, symbols_)) {
      undefined_symbols_ = true;
      Unload();
      return false;
    }
    return true;
  }

  void Unload() {
    if (!IsLoaded()) {
      return;
    }
    InternalUnloadDll(handle_);
    handle_ = kInvalidDllHandle;
    memset(symbols_, 0, sizeof(symbols_));
  }

  
  
  void *GetSymbol(int index) const {
    assert(IsLoaded());
    assert(index < NumSymbols());
    return symbols_[index];
  }

 private:
  DllHandle handle_;
  bool undefined_symbols_;
  void *symbols_[SYMBOL_TABLE_SIZE];

  DISALLOW_COPY_AND_ASSIGN(LateBindingSymbolTable);
};


#define LATE_BINDING_SYMBOL_TABLE_DECLARE_BEGIN(ClassName) \
enum {






#define LATE_BINDING_SYMBOL_TABLE_DECLARE_ENTRY(ClassName, sym) \
  ClassName##_SYMBOL_TABLE_INDEX_##sym,


#define LATE_BINDING_SYMBOL_TABLE_DECLARE_END(ClassName) \
  ClassName##_SYMBOL_TABLE_SIZE \
}; \
\
extern const char ClassName##_kDllName[]; \
extern const char *const \
    ClassName##_kSymbolNames[ClassName##_SYMBOL_TABLE_SIZE]; \
\
typedef ::webrtc_adm_linux::LateBindingSymbolTable<ClassName##_SYMBOL_TABLE_SIZE, \
                                            ClassName##_kDllName, \
                                            ClassName##_kSymbolNames> \
    ClassName;



#define LATE_BINDING_SYMBOL_TABLE_DEFINE_BEGIN(ClassName, dllName) \
const char ClassName##_kDllName[] = dllName; \
const char *const ClassName##_kSymbolNames[ClassName##_SYMBOL_TABLE_SIZE] = {





#define LATE_BINDING_SYMBOL_TABLE_DEFINE_ENTRY(ClassName, sym) \
  #sym,

#define LATE_BINDING_SYMBOL_TABLE_DEFINE_END(ClassName) \
};


#define LATESYM_INDEXOF(ClassName, sym) \
  (ClassName##_SYMBOL_TABLE_INDEX_##sym)


#define LATESYM_GET(ClassName, inst, sym) \
  (*reinterpret_cast<typeof(&sym)>( \
      (inst)->GetSymbol(LATESYM_INDEXOF(ClassName, sym))))

}  

#endif  
