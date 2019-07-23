














































#ifndef nsIJVMPluginTagInfo_h___
#define nsIJVMPluginTagInfo_h___

#include "nsISupports.h"






#define NS_IJVMPLUGINTAGINFO_IID                     \
{ /* 27b42df0-a1bd-11d1-85b1-00805f0e4dfe */         \
    0x27b42df0,                                      \
    0xa1bd,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}

class nsIJVMPluginTagInfo : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJVMPLUGINTAGINFO_IID)

    NS_IMETHOD
    GetCode(const char* *result) = 0;

    NS_IMETHOD
    GetCodeBase(const char* *result) = 0;

    NS_IMETHOD
    GetArchive(const char* *result) = 0;

    NS_IMETHOD
    GetName(const char* *result) = 0;

    NS_IMETHOD
    GetMayScript(PRBool *result) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJVMPluginTagInfo, NS_IJVMPLUGINTAGINFO_IID)



#endif 
