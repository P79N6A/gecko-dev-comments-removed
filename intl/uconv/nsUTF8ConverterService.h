



#ifndef nsUTF8ConverterService_h__
#define nsUTF8ConverterService_h__

#include "nsIUTF8ConverterService.h"



class nsUTF8ConverterService: public nsIUTF8ConverterService {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUTF8CONVERTERSERVICE

  nsUTF8ConverterService() {}

private:
  virtual ~nsUTF8ConverterService() {}
};

#endif 

