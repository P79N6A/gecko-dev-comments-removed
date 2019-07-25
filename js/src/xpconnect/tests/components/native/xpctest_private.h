









































#ifndef xpctest_private_h___
#define xpctest_private_h___

#include "nsISupports.h"
#include "nsMemory.h"
#include "jsapi.h"
#include "nsStringGlue.h"
#include "xpctest_attributes.h"
#include "xpctest_params.h"

class xpcTestObjectReadOnly : public nsIXPCTestObjectReadOnly {
 public: 
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTOBJECTREADONLY
  xpcTestObjectReadOnly();
   
 private:
    PRBool  boolProperty;
    PRInt16 shortProperty;
    PRInt32 longProperty;
    float   floatProperty;
    char    charProperty;
};

class xpcTestObjectReadWrite : public nsIXPCTestObjectReadWrite {
  public: 
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTOBJECTREADWRITE

  xpcTestObjectReadWrite();
  ~xpcTestObjectReadWrite();

 private:
     PRBool boolProperty;
     PRInt16 shortProperty;
     PRInt32 longProperty;
     float floatProperty;
     char charProperty;
     char *stringProperty;
};

class nsXPCTestParams : public nsIXPCTestParams
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTPARAMS

    nsXPCTestParams();

private:
    ~nsXPCTestParams();
};

#endif 
