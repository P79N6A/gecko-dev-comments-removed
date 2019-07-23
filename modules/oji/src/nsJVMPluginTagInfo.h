




































#ifndef nsJVMPluginTagInfo_h___
#define nsJVMPluginTagInfo_h___

#include "nsIJVMPluginTagInfo.h"
#include "nsAgg.h"






class nsIPluginTagInfo2;

class nsJVMPluginTagInfo : public nsIJVMPluginTagInfo {
public:

    NS_DECL_AGGREGATED

    

    

    NS_IMETHOD
    GetCode(const char* *result);

    NS_IMETHOD
    GetCodeBase(const char* *result);

    NS_IMETHOD
    GetArchive(const char* *result);

    NS_IMETHOD
    GetName(const char* *result);

    NS_IMETHOD
    GetMayScript(PRBool *result);

    
    
    

    static NS_METHOD
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr,
           nsIPluginTagInfo2* info);

protected:

    nsJVMPluginTagInfo(nsISupports* outer, nsIPluginTagInfo2* info);
    virtual ~nsJVMPluginTagInfo(void);

    
    nsIPluginTagInfo2*  fPluginTagInfo;
    char*               fSimulatedCodebase;
    char*               fSimulatedCode;
};

#endif 
