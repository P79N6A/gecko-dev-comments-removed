





#ifndef nsISizeOf_h___
#define nsISizeOf_h___

#include "nsISupports.h"

#define NS_ISIZEOF_IID \
  {0x61d05579, 0xd7ec, 0x485c, \
    { 0xa4, 0x0c, 0x31, 0xc7, 0x9a, 0x5c, 0xf9, 0xf3 }}

class nsISizeOf : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISIZEOF_IID)

  


  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const = 0;

  


  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISizeOf, NS_ISIZEOF_IID)

#endif 
