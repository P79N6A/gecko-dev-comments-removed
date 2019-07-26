




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
    MIME_FIELD_ENCODING = 1,
    HTTP_FIELD_ENCODING
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

