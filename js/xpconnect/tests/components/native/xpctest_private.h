







#ifndef xpctest_private_h___
#define xpctest_private_h___

#include "nsISupports.h"
#include "nsMemory.h"
#include "nsStringGlue.h"
#include "xpctest_attributes.h"
#include "xpctest_params.h"
#include "xpctest_returncode.h"
#include "mozilla/Attributes.h"

class xpcTestObjectReadOnly final : public nsIXPCTestObjectReadOnly {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTOBJECTREADONLY
  xpcTestObjectReadOnly();

 private:
    ~xpcTestObjectReadOnly() {}

    bool    boolProperty;
    int16_t shortProperty;
    int32_t longProperty;
    float   floatProperty;
    char    charProperty;
    PRTime  timeProperty;
};

class xpcTestObjectReadWrite final : public nsIXPCTestObjectReadWrite {
  public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTOBJECTREADWRITE

  xpcTestObjectReadWrite();

 private:
     ~xpcTestObjectReadWrite();

     bool boolProperty;
     int16_t shortProperty;
     int32_t longProperty;
     float floatProperty;
     char charProperty;
     char* stringProperty;
     PRTime timeProperty;
};

class nsXPCTestParams final : public nsIXPCTestParams
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTPARAMS

    nsXPCTestParams();

private:
    ~nsXPCTestParams();
};

class nsXPCTestReturnCodeParent final : public nsIXPCTestReturnCodeParent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTRETURNCODEPARENT

    nsXPCTestReturnCodeParent();

private:
    ~nsXPCTestReturnCodeParent();
};

#endif 
