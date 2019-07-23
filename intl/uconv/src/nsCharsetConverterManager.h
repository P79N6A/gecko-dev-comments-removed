



































#ifndef nsCharsetConverterManager_h__
#define nsCharsetConverterManager_h__

#include "nsISupports.h"
#include "nsICharsetConverterManager.h"
#include "nsIStringBundle.h"
#include "nsInterfaceHashtable.h"
#include "mozilla/Mutex.h"

#ifdef MOZ_USE_NATIVE_UCONV
#include "nsINativeUConvService.h"
#endif

class nsCharsetConverterManager : public nsICharsetConverterManager
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHARSETCONVERTERMANAGER

public:

  nsCharsetConverterManager();
  virtual ~nsCharsetConverterManager();

private:

  nsIStringBundle * mDataBundle;
  nsIStringBundle * mTitleBundle;

#ifdef MOZ_USE_NATIVE_UCONV
  nsCOMPtr<nsINativeUConvService> mNativeUC;
#endif

  nsresult LoadExtensibleBundle(const char * aRegistryKey, 
      nsIStringBundle ** aResult);

  static nsresult RegisterConverterCategory(nsICategoryManager*,
                                            const char* aCategory,
                                            const char* aURL);

  nsresult GetBundleValue(nsIStringBundle * aBundle,
                          const char * aName, 
                          const nsAFlatString& aProp, PRUnichar ** aResult);
  nsresult GetBundleValue(nsIStringBundle * aBundle,
                          const char * aName, 
                          const nsAFlatString& aProp, nsAString& aResult);

  nsresult GetList(const nsACString& aCategory,
                   const nsACString& aPrefix,
                   nsIUTF8StringEnumerator** aResult);

public:
  static nsresult RegisterConverterManagerData();

};

#endif 


