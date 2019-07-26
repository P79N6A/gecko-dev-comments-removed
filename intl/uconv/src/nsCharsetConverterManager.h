



#ifndef nsCharsetConverterManager_h__
#define nsCharsetConverterManager_h__

#include "nsISupports.h"
#include "nsICharsetConverterManager.h"

class nsCharsetAlias;

class nsCharsetConverterManager : public nsICharsetConverterManager
{
  friend class nsCharsetAlias;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICHARSETCONVERTERMANAGER

public:
  nsCharsetConverterManager();
  virtual ~nsCharsetConverterManager();

  static void Shutdown();

private:

  static bool IsInternal(const nsACString& aCharset);
};

#endif 


