















#include "nsOSHelperAppService.h"
#include "nsMIMEInfoImpl.h"



class nsGonkMIMEInfo : public nsMIMEInfoImpl {
protected:
    virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
};

nsOSHelperAppService::nsOSHelperAppService() : nsExternalHelperAppService()
{
}

nsOSHelperAppService::~nsOSHelperAppService()
{
}

already_AddRefed<nsIMIMEInfo>
nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aMIMEType,
                                        const nsACString& aFileExt,
                                        bool* aFound)
{
    *aFound = false;
    
    
    nsRefPtr<nsGonkMIMEInfo> mimeInfo = new nsGonkMIMEInfo();
    return mimeInfo.forget();
}

nsresult
nsOSHelperAppService::OSProtocolHandlerExists(const char* aScheme,
                                              bool* aExists)
{
    *aExists = false;
    return NS_OK;
}
