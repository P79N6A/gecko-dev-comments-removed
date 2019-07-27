



#ifndef nsUTF8ConverterService_h__
#define nsUTF8ConverterService_h__

#include "nsIUTF8ConverterService.h"



class nsUTF8ConverterService;

namespace mozilla {
template<>
struct HasDangerousPublicDestructor<nsUTF8ConverterService>
{
  static const bool value = true;
};
}

class nsUTF8ConverterService: public nsIUTF8ConverterService {
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUTF8CONVERTERSERVICE

public:
  nsUTF8ConverterService() {}
  virtual ~nsUTF8ConverterService() {}
};

#endif 

