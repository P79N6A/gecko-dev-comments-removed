




































#include "nsOSHelperAppService.h"
#include "nsMIMEInfoAndroid.h"
#include "AndroidBridge.h"

nsOSHelperAppService::nsOSHelperAppService() : nsExternalHelperAppService()
{
}

nsOSHelperAppService::~nsOSHelperAppService()
{
}

already_AddRefed<nsIMIMEInfo>
nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aMIMEType,
                                        const nsACString& aFileExt,
                                        PRBool* aFound)
{
    
    if (!mozilla::AndroidBridge::Bridge())
        return nsnull;

    *aFound = PR_FALSE;
    already_AddRefed<nsIMIMEInfo> mimeInfo = 
            nsMIMEInfoAndroid::GetMimeInfoForMimeType(aMIMEType);
    if (!mimeInfo.get())
            mimeInfo = nsMIMEInfoAndroid::GetMimeInfoForFileExt(aFileExt);

    *aFound = !!mimeInfo.get();
    
    return mimeInfo;
}

nsresult
nsOSHelperAppService::OSProtocolHandlerExists(const char* aScheme,
                                              PRBool* aExists)
{
    *aExists = PR_FALSE;
    return NS_OK;
}
