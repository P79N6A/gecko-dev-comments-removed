




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

   
   static bool Compose(uint32_t a, uint32_t b, uint32_t *ab);
   static bool DecomposeNonRecursively(uint32_t comp, uint32_t *c1, uint32_t *c2);
};

#endif 

