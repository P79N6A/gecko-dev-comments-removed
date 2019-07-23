




































#ifndef _nsAppFileLocProviderProxy_h_
#define _nsAppFileLocProviderProxy_h_

#include "nsIDirectoryService.h"
#include "jni.h"


class nsAppFileLocProviderProxy : public nsIDirectoryServiceProvider2
{
public:
  nsAppFileLocProviderProxy(jobject aJavaLocProvider);
  ~nsAppFileLocProviderProxy();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

private:
  jobject   mJavaLocProvider;
};

extern "C" nsresult
NS_NewAppFileLocProviderProxy(jobject aJavaLocProvider,
                              nsIDirectoryServiceProvider** aResult);


#endif  

