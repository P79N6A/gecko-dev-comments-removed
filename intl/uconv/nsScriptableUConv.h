





#ifndef __nsScriptableUConv_h_
#define __nsScriptableUConv_h_

#include "nsIScriptableUConv.h"
#include "nsCOMPtr.h"
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"

class nsScriptableUnicodeConverter : public nsIScriptableUnicodeConverter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTABLEUNICODECONVERTER

  nsScriptableUnicodeConverter();

protected:
  virtual ~nsScriptableUnicodeConverter();

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
