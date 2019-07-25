




































#include "nsIMIMEHeaderParam.h"

#ifndef __nsmimeheaderparamimpl_h___
#define __nsmimeheaderparamimpl_h___
class nsMIMEHeaderParamImpl : public nsIMIMEHeaderParam
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMIMEHEADERPARAM

  nsMIMEHeaderParamImpl() {}
  virtual ~nsMIMEHeaderParamImpl() {}
private:
  
  
  enum ParamDecoding {
    RFC_2231_DECODING = 1,
    RFC_5987_DECODING
  }; 

  nsresult DoGetParameter(const nsACString& aHeaderVal, 
                          const char *aParamName,
                          ParamDecoding aDecoding,
                          const nsACString& aFallbackCharset, 
                          bool aTryLocaleCharset, 
                          char **aLang, 
                          nsAString& aResult);

  nsresult DoParameterInternal(const char *aHeaderValue, 
                               const char *aParamName,
                               ParamDecoding aDecoding,
                               char **aCharset,
                               char **aLang,
                               char **aResult);

};

#endif 

