




































#include "nsOSHelperAppService.h"

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
    return nsnull;
}

nsresult
nsOSHelperAppService::OSProtocolHandlerExists(const char* aScheme,
                                              bool* aExists)
{
    *aExists = false;
    return NS_OK;
}
