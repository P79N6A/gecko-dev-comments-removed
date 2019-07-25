




































#ifndef mozilla_Module_h
#define mozilla_Module_h

#include "nscore.h"
#include "nsID.h"
#include "nsIFactory.h"
#include "nsCOMPtr.h" 

namespace mozilla {







struct Module
{
  static const int kVersion = 1;

  struct CIDEntry;

  typedef already_AddRefed<nsIFactory> (*GetFactoryProc)
    (const Module& module, const CIDEntry& entry);

  typedef nsresult (*ConstructorProc)(nsISupports* aOuter,
                                      const nsIID& aIID,
                                      void** aResult);

  typedef nsresult (*LoadedFunc)();
  typedef void (*UnloadedFunc)();

  



  struct CIDEntry
  {
    const nsCID* cid;
    bool service;
    GetFactoryProc getfactory;
    ConstructorProc constructor;
  };

  struct ContractIDEntry
  {
    const char* contractid;
    nsID const * cid;
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

  






  GetFactoryProc getfactory;

  




  LoadedFunc loaded;
  UnloadedFunc unloaded;
};

} 

#if defined(XPCOM_TRANSLATE_NSGM_ENTRY_POINT)
#  define NSMODULE_NAME(_name) _name##_NSModule
#  define NSMODULE_DECL(_name) extern mozilla::Module const *const NSMODULE_NAME(_name)
#  define NSMODULE_DEFN(_name) NSMODULE_DECL(_name)
#else
#  define NSMODULE_NAME(_name) NSModule
#  define NSMODULE_DEFN(_name) extern "C" NS_EXPORT mozilla::Module const *const NSModule
#endif

#endif 
