





#ifndef mozilla_Module_h
#define mozilla_Module_h

#include "nscore.h"
#include "nsID.h"
#include "nsIFactory.h"
#include "nsCOMPtr.h" 

namespace mozilla {







struct Module
{
  static const unsigned int kVersion = 40;

  struct CIDEntry;

  typedef already_AddRefed<nsIFactory> (*GetFactoryProcPtr)(
    const Module& module, const CIDEntry& entry);

  typedef nsresult (*ConstructorProcPtr)(nsISupports* aOuter,
                                         const nsIID& aIID,
                                         void** aResult);

  typedef nsresult (*LoadFuncPtr)();
  typedef void (*UnloadFuncPtr)();

  



  enum ProcessSelector
  {
    ANY_PROCESS = 0,
    MAIN_PROCESS_ONLY,
    CONTENT_PROCESS_ONLY
  };

  



  struct CIDEntry
  {
    const nsCID* cid;
    bool service;
    GetFactoryProcPtr getFactoryProc;
    ConstructorProcPtr constructorProc;
    ProcessSelector processSelector;
  };

  struct ContractIDEntry
  {
    const char* contractid;
    nsID const* cid;
    ProcessSelector processSelector;
  };

  struct CategoryEntry
  {
    const char* category;
    const char* entry;
    const char* value;
  };

  


  unsigned int mVersion;

  



  const CIDEntry* mCIDs;

  



  const ContractIDEntry* mContractIDs;

  



  const CategoryEntry* mCategoryEntries;

  






  GetFactoryProcPtr getFactoryProc;

  




  LoadFuncPtr loadProc;
  UnloadFuncPtr unloadProc;
};

} 

#if defined(MOZILLA_INTERNAL_API)
#  define NSMODULE_NAME(_name) _name##_NSModule
#  if defined(_MSC_VER)
#    pragma section(".kPStaticModules$M", read)
#    pragma comment(linker, "/merge:.kPStaticModules=.rdata")
#    define NSMODULE_SECTION __declspec(allocate(".kPStaticModules$M"), dllexport)
#  elif defined(__GNUC__)
#    if defined(__ELF__)
#      define NSMODULE_SECTION __attribute__((section(".kPStaticModules"), visibility("protected")))
#    elif defined(__MACH__)
#      define NSMODULE_SECTION __attribute__((section("__DATA, .kPStaticModules"), visibility("default")))
#    elif defined (_WIN32)
#      define NSMODULE_SECTION __attribute__((section(".kPStaticModules"), dllexport))
#    endif
#  endif
#  if !defined(NSMODULE_SECTION)
#    error Do not know how to define sections.
#  endif
#  define NSMODULE_DEFN(_name) extern NSMODULE_SECTION mozilla::Module const *const NSMODULE_NAME(_name)
#else
#  define NSMODULE_NAME(_name) NSModule
#  define NSMODULE_DEFN(_name) extern "C" NS_EXPORT mozilla::Module const *const NSModule
#endif

#endif 
