



#ifndef nsNetStrings_h__
#define nsNetStrings_h__

#include "nsLiteralString.h"





class nsNetStrings {
public:
  nsNetStrings();

  const nsLiteralString kChannelPolicy;
};

extern nsNetStrings* gNetStrings;


#endif
