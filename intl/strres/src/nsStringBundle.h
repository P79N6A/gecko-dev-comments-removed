




































#ifndef nsStringBundle_h__
#define nsStringBundle_h__

#include "mozilla/ReentrantMonitor.h"
#include "nsIStringBundle.h"
#include "nsCOMPtr.h"
#include "nsIPersistentProperties2.h"
#include "nsString.h"
#include "nsCOMArray.h"
#include "nsIStringBundleOverride.h"

class nsStringBundle : public nsIStringBundle
{
public:
    
    nsStringBundle(const char* aURLSpec, nsIStringBundleOverride*);
    nsresult LoadProperties();
    virtual ~nsStringBundle();
  
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTRINGBUNDLE

    nsCOMPtr<nsIPersistentProperties> mProps;

protected:
    
    
    
    nsresult GetStringFromID(PRInt32 aID, nsAString& aResult);
    nsresult GetStringFromName(const nsAString& aName, nsAString& aResult);

    nsresult GetCombinedEnumeration(nsIStringBundleOverride* aOverrideString,
                                    nsISimpleEnumerator** aResult);
private:
    nsCString              mPropertiesURL;
    nsCOMPtr<nsIStringBundleOverride> mOverrideStrings;
    mozilla::ReentrantMonitor    mReentrantMonitor;
    PRPackedBool                 mAttemptedLoad;
    PRPackedBool                 mLoaded;
    
public:
    static nsresult FormatString(const PRUnichar *formatStr,
                                 const PRUnichar **aParams, PRUint32 aLength,
                                 PRUnichar **aResult);
};







class nsExtensibleStringBundle : public nsIStringBundle
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGBUNDLE

  nsresult Init(const char * aCategory, nsIStringBundleService *);
private:
  
  nsCOMArray<nsIStringBundle> mBundles;
  PRBool             mLoaded;

public:

  nsExtensibleStringBundle();
  virtual ~nsExtensibleStringBundle();
};



#endif
