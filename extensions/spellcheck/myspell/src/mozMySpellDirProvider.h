




































#ifndef mozMySpellDirProvider_h__
#define mozMySpellDirProvider_h__

#include "nsIDirectoryService.h"
#include "nsIGenericFactory.h"
#include "nsISimpleEnumerator.h"

class mozMySpellDirProvider :
  public nsIDirectoryServiceProvider2
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

  static NS_METHOD Register(nsIComponentManager* aCompMgr,
			    nsIFile* aPath, const char *aLoaderStr,
			    const char *aType,
			    const nsModuleComponentInfo *aInfo);

  static NS_METHOD Unregister(nsIComponentManager* aCompMgr,
			      nsIFile* aPath, const char *aLoaderStr,
			      const nsModuleComponentInfo *aInfo);

  static char const *const kContractID;

private:
  class AppendingEnumerator : public nsISimpleEnumerator
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR

    AppendingEnumerator(nsISimpleEnumerator* aBase);

  private:
    nsCOMPtr<nsISimpleEnumerator> mBase;
    nsCOMPtr<nsIFile>             mNext;
  };
};

#define MYSPELLDIRPROVIDER_CID \
{ 0x64d6174c, 0x1496, 0x4ffd, \
  { 0x87, 0xf2, 0xda, 0x26, 0x70, 0xf8, 0x89, 0x34 } }

#endif 
