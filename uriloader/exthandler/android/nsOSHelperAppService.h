




































#ifndef nsOSHelperAppService_h
#define nsOSHelperAppService_h

#include "nsCExternalHandlerService.h"
#include "nsExternalHelperAppService.h"

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
    nsOSHelperAppService();
    virtual ~nsOSHelperAppService();

    virtual already_AddRefed<nsIMIMEInfo>
    GetMIMEInfoFromOS(const nsACString& aMIMEType,
                      const nsACString& aFileExt,
                      PRBool* aFound);

    virtual NS_HIDDEN_(nsresult)
    OSProtocolHandlerExists(const char* aScheme,
                            PRBool* aExists);

    NS_IMETHOD GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                            PRBool *found,
                                            nsIHandlerInfo **_retval);

    static nsIHandlerApp*
    CreateAndroidHandlerApp(const nsAString& aName,
                            const nsAString& aDescription,
                            const nsAString& aPackageName,
                            const nsAString& aClassName, 
                            const nsACString& aMimeType,
                            const nsAString& aAction = EmptyString());
};

#endif 
