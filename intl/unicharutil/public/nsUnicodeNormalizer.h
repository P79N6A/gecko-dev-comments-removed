




































#ifndef nsUnicodeNormalizer_h__
#define nsUnicodeNormalizer_h__

#include "nscore.h"
#include "nsISupports.h"

#include "nsIUnicodeNormalizer.h"

nsresult NS_NewUnicodeNormalizer(nsISupports** oResult);


class nsUnicodeNormalizer : public nsIUnicodeNormalizer {
public:
   nsUnicodeNormalizer();
   virtual ~nsUnicodeNormalizer();

   NS_DECL_ISUPPORTS 

   NS_IMETHOD NormalizeUnicodeNFD( const nsAString& aSrc, nsAString& aDest);
   NS_IMETHOD NormalizeUnicodeNFC( const nsAString& aSrc, nsAString& aDest);
   NS_IMETHOD NormalizeUnicodeNFKD( const nsAString& aSrc, nsAString& aDest);
   NS_IMETHOD NormalizeUnicodeNFKC( const nsAString& aSrc, nsAString& aDest);

   
   static bool Compose(PRUint32 a, PRUint32 b, PRUint32 *ab);
   static bool DecomposeNonRecursively(PRUint32 comp, PRUint32 *c1, PRUint32 *c2);
};

#endif 

