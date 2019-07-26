















#include "nsOSHelperAppService.h"
#include "nsMIMEInfoImpl.h"

class nsGonkMIMEInfo : public nsMIMEInfoImpl {
public:
    nsGonkMIMEInfo(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) { }

protected:
    virtual nsresult LoadUriInternal(nsIURI *aURI) {
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
    
    
    nsRefPtr<nsGonkMIMEInfo> mimeInfo = new nsGonkMIMEInfo(aMIMEType);
    return mimeInfo.forget();
}

nsresult
nsOSHelperAppService::OSProtocolHandlerExists(const char* aScheme,
                                              bool* aExists)
{
    *aExists = false;
    return NS_OK;
}
