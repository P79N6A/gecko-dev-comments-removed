




































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
                                        bool* aFound)
{
    nsRefPtr<nsMIMEInfoAndroid> mimeInfo;
    *aFound = PR_FALSE;
    if (!aMIMEType.IsEmpty())
        *aFound = 
            nsMIMEInfoAndroid::GetMimeInfoForMimeType(aMIMEType, 
                                                      getter_AddRefs(mimeInfo));
    if (!*aFound)
        *aFound =
            nsMIMEInfoAndroid::GetMimeInfoForFileExt(aFileExt, 
                                                     getter_AddRefs(mimeInfo));

    
    
    if (!*aFound)
        mimeInfo = new nsMIMEInfoAndroid(aMIMEType);

    return mimeInfo.forget();
}

nsresult
nsOSHelperAppService::OSProtocolHandlerExists(const char* aScheme,
                                              bool* aExists)
{
    *aExists = mozilla::AndroidBridge::Bridge()->GetHandlersForURL(aScheme);    
    return NS_OK;
}

nsresult nsOSHelperAppService::GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                      bool *found,
                                      nsIHandlerInfo **info)
{
    return nsMIMEInfoAndroid::GetMimeInfoForURL(aScheme, found, info);
}

nsIHandlerApp*
nsOSHelperAppService::CreateAndroidHandlerApp(const nsAString& aName,
                                              const nsAString& aDescription,
                                              const nsAString& aPackageName,
                                              const nsAString& aClassName, 
                                              const nsACString& aMimeType,
                                              const nsAString& aAction)
{
    return new nsAndroidHandlerApp(aName, aDescription, aPackageName,
                                   aClassName, aMimeType, aAction);
}
