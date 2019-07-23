



































#ifndef nsCharsetAlias_h__
#define nsCharsetAlias_h__

#include "nsICharsetAlias.h"
#include "nsGREResProperties.h"
#include "mozilla/Mutex.h"


class nsCharsetAlias2 : public nsICharsetAlias
{
  NS_DECL_ISUPPORTS

public:

  nsCharsetAlias2();
  virtual ~nsCharsetAlias2();

  NS_IMETHOD GetPreferred(const nsACString& aAlias, nsACString& aResult);

  NS_IMETHOD Equals(const nsACString& aCharset1, const nsACString& aCharset2, PRBool* oResult) ;
  
private:
  nsGREResProperties* mDelegate;
  mozilla::Mutex      mDelegateMutex;
};

#endif 


