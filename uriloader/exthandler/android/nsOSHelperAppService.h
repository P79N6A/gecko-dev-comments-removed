




































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
};

#endif 
