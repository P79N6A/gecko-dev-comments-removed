





#ifndef mozilla_ModuleLoader_h
#define mozilla_ModuleLoader_h

#include "nsISupports.h"
#include "mozilla/Module.h"
#include "mozilla/FileLocation.h"

#define MOZILLA_MODULELOADER_PSEUDO_IID \
{ 0xD951A8CE, 0x6E9F, 0x464F, \
  { 0x8A, 0xC8, 0x14, 0x61, 0xC0, 0xD3, 0x63, 0xC8 } }

namespace mozilla {








class ModuleLoader : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_MODULELOADER_PSEUDO_IID)

  





  virtual const Module* LoadModule(mozilla::FileLocation& aFile) = 0;
};
NS_DEFINE_STATIC_IID_ACCESSOR(ModuleLoader, MOZILLA_MODULELOADER_PSEUDO_IID)

} 

#endif 
