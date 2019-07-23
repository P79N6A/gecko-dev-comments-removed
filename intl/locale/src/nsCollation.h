





































#ifndef nsCollation_h__
#define nsCollation_h__


#include "nsICollation.h"
#include "nsICaseConversion.h"
#include "nsICharsetConverterManager.h"
#include "nsCOMPtr.h"




class nsCollationFactory: public nsICollationFactory {

public: 
  NS_DECL_ISUPPORTS 

  NS_IMETHOD CreateCollation(nsILocale* locale, nsICollation** instancePtr);

  nsCollationFactory() {}
};


struct nsCollation {

public: 

  nsCollation();
  
  ~nsCollation();

  
  nsresult NormalizeString(const nsAString& stringIn, nsAString& stringOut);

  
  nsresult SetCharset(const char* aCharset);
  nsresult UnicodeToChar(const nsAString& aSrc, char** dst);

protected:
  nsCOMPtr <nsICaseConversion>            mCaseConversion;
  nsCOMPtr <nsIUnicodeEncoder>            mEncoder;
};

#endif  
