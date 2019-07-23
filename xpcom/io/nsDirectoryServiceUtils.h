





































#ifndef nsDirectoryServiceUtils_h___
#define nsDirectoryServiceUtils_h___

#include "nsIServiceManager.h"
#include "nsIProperties.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsXPCOMCID.h"
#include "nsIFile.h"

inline nsresult
NS_GetSpecialDirectory(const char* specialDirName, nsIFile* *result)
{
    nsresult rv;
    nsCOMPtr<nsIProperties> serv(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv))
        return rv;

    return serv->Get(specialDirName, NS_GET_IID(nsIFile),
                     reinterpret_cast<void**>(result));
}

#endif
