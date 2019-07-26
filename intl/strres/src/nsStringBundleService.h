




#ifndef nsStringBundleService_h__
#define nsStringBundleService_h__

#include "nsCOMPtr.h"
#include "nsHashtable.h"
#include "nsIPersistentProperties2.h"
#include "nsIStringBundle.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIErrorService.h"
#include "nsIStringBundleOverride.h"

#include "mozilla/LinkedList.h"

struct bundleCacheEntry_t;

class nsStringBundleService : public nsIStringBundleService,
                              public nsIObserver,
                              public nsSupportsWeakReference
{
public:
  nsStringBundleService();
  virtual ~nsStringBundleService();

  nsresult Init();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTRINGBUNDLESERVICE
  NS_DECL_NSIOBSERVER

private:
  nsresult getStringBundle(const char *aUrl, nsIStringBundle** aResult);
  nsresult FormatWithBundle(nsIStringBundle* bundle, nsresult aStatus,
                            uint32_t argCount, char16_t** argArray,
                            char16_t* *result);

  void flushBundleCache();

  bundleCacheEntry_t *insertIntoCache(already_AddRefed<nsIStringBundle> aBundle,
                                      nsCStringKey *aHashKey);

  nsHashtable mBundleMap;
  mozilla::LinkedList<bundleCacheEntry_t> mBundleCache;

  nsCOMPtr<nsIErrorService>     mErrorService;
  nsCOMPtr<nsIStringBundleOverride> mOverrideStrings;
};

#endif
