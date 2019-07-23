




































#define NS_NATIVE_UCONV_SERVICE_CONTRACT_ID "@mozilla.org/uconv/native-service"

#define NS_NATIVE_UCONV_SERVICE_CID \
{ 0xbd3e94ba, 0xd46f, 0x4026, \
{ 0xa1, 0xc3, 0x6e, 0xd0, 0xc1, 0x6e, 0xa0, 0x22 } }

#include "nsINativeUConvService.h"

class NativeUConvService : public nsINativeUConvService
{
public:

    NativeUConvService() {};
    virtual ~NativeUConvService() {};

    NS_DECL_ISUPPORTS
    NS_DECL_NSINATIVEUCONVSERVICE
};

