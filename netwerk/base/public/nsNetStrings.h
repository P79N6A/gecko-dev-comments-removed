



































#ifndef nsNetStrings_h__
#define nsNetStrings_h__

#include "nsLiteralString.h"





class nsNetStrings {
public:
  nsNetStrings();

  
  const nsLiteralString kContentLength;
  const nsLiteralString kContentDisposition;
  const nsLiteralString kChannelPolicy;
};

extern NS_HIDDEN_(nsNetStrings*) gNetStrings;


#endif
