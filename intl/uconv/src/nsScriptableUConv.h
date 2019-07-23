







































#ifndef __nsScriptableUConv_h_
#define __nsScriptableUConv_h_

#include "nsICharsetConverterManager.h"
#include "nsIScriptableUConv.h"

class nsScriptableUnicodeConverter : public nsIScriptableUnicodeConverter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTABLEUNICODECONVERTER

  nsScriptableUnicodeConverter();
  virtual ~nsScriptableUnicodeConverter();

protected:
  
  
  nsCAutoString mCharset;
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;

  nsresult FinishWithLength(char **_retval, PRInt32* aLength);
  nsresult ConvertFromUnicodeWithLength(const nsAString& aSrc,
                                        PRInt32* aOutLen,
                                        char **_retval);


  nsresult InitConverter();
};

#endif
