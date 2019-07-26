




#ifndef nsXULTemplateResultStorage_h__
#define nsXULTemplateResultStorage_h__

#include "nsXULTemplateQueryProcessorStorage.h"
#include "nsIRDFResource.h"
#include "nsIXULTemplateResult.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"




class nsXULTemplateResultStorage MOZ_FINAL : public nsIXULTemplateResult
{
public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIXULTEMPLATERESULT

    nsXULTemplateResultStorage(nsXULTemplateResultSetStorage* aResultSet);

protected:

    ~nsXULTemplateResultStorage();

    nsRefPtr<nsXULTemplateResultSetStorage> mResultSet;

    nsCOMArray<nsIVariant> mValues;

    nsCOMPtr<nsIRDFResource> mNode;
};

#endif 
