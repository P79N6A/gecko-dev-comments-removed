




































#ifndef DirectoryProvider_h__
#define DirectoryProvider_h__

#include "nsIDirectoryService.h"
#include "nsComponentManagerUtils.h"
#include "nsISimpleEnumerator.h"
#include "nsIFile.h"

#define NS_BROWSERDIRECTORYPROVIDER_CONTRACTID \
  "@mozilla.org/browser/directory-provider;1"

namespace mozilla {
namespace browser {

class DirectoryProvider : public nsIDirectoryServiceProvider2
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

private:
  class AppendingEnumerator : public nsISimpleEnumerator
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR

    AppendingEnumerator(nsISimpleEnumerator* aBase,
                        char const *const *aAppendList);

  private:
    nsCOMPtr<nsISimpleEnumerator> mBase;
    char const *const *const      mAppendList;
    nsCOMPtr<nsIFile>             mNext;
  };
};

} 
} 

#endif 
