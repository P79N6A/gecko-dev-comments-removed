





































#ifndef nsArray_h__
#define nsArray_h__

#include "nsIMutableArray.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"

#define NS_ARRAY_CLASSNAME \
  "nsIArray implementation"


#define NS_ARRAY_CID \
{ 0x35c66fd1, 0x95e9, 0x4e0a, \
  { 0x80, 0xc5, 0xc3, 0xbd, 0x2b, 0x37, 0x54, 0x81 } }






class nsArray : public nsIMutableArray
{
public:
    nsArray() { }
    nsArray(const nsCOMArray_base& aBaseArray) : mArray(aBaseArray)
    { }
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIARRAY
    NS_DECL_NSIMUTABLEARRAY

private:
    ~nsArray();

    nsCOMArray_base mArray;
};

#endif
