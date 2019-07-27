




#ifndef nsStringBundle_h__
#define nsStringBundle_h__

#include "mozilla/ReentrantMonitor.h"
#include "nsIStringBundle.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsCOMArray.h"

class nsIPersistentProperties;
class nsIStringBundleOverride;

class nsStringBundle : public nsIStringBundle
{
public:
    
    nsStringBundle(const char* aURLSpec, nsIStringBundleOverride*);
    nsresult LoadProperties();

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSISTRINGBUNDLE

    nsCOMPtr<nsIPersistentProperties> mProps;

protected:
    virtual ~nsStringBundle();

    
    
    
    nsresult GetStringFromID(int32_t aID, nsAString& aResult);
    nsresult GetStringFromName(const nsAString& aName, nsAString& aResult);

    nsresult GetCombinedEnumeration(nsIStringBundleOverride* aOverrideString,
                                    nsISimpleEnumerator** aResult);
private:
    nsCString              mPropertiesURL;
    nsCOMPtr<nsIStringBundleOverride> mOverrideStrings;
    mozilla::ReentrantMonitor    mReentrantMonitor;
    bool                         mAttemptedLoad;
    bool                         mLoaded;

public:
    static nsresult FormatString(const char16_t *formatStr,
                                 const char16_t **aParams, uint32_t aLength,
                                 char16_t **aResult);
};

class nsExtensibleStringBundle;







class nsExtensibleStringBundle MOZ_FINAL : public nsIStringBundle
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGBUNDLE

  nsresult Init(const char * aCategory, nsIStringBundleService *);

public:
  nsExtensibleStringBundle();

private:
  virtual ~nsExtensibleStringBundle();

  nsCOMArray<nsIStringBundle> mBundles;
  bool mLoaded;
};



#endif
