




































#ifndef nsUConvPropertySearch_h_
#define nsUConvPropertySearch_h_

#include "nsString.h"

class nsUConvPropertySearch
{
  public:
    










    static nsresult SearchPropertyValue(const char* aProperties[][3],
                                        PRInt32 aNumberOfProperties,
                                        const nsACString& aKey,
                                        nsACString& aValue);
};

#endif 
