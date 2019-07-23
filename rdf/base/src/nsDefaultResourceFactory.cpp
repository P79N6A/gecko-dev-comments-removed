












































#include "nsRDFResource.h"

nsresult
NS_NewDefaultResource(nsIRDFResource** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsRDFResource* resource = new nsRDFResource();
    if (! resource)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(resource);
    *aResult = resource;
    return NS_OK;
}
