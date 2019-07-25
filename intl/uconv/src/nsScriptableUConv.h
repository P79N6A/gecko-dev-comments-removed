





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
  nsCString mCharset;
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  bool mIsInternal;

  nsresult FinishWithLength(char **_retval, int32_t* aLength);
  nsresult ConvertFromUnicodeWithLength(const nsAString& aSrc,
                                        int32_t* aOutLen,
                                        char **_retval);


  nsresult InitConverter();
};

#endif
