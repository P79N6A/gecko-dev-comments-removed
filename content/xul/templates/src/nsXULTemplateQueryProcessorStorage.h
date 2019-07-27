




#ifndef nsXULTemplateQueryProcessorStorage_h__
#define nsXULTemplateQueryProcessorStorage_h__

#include "nsIXULTemplateBuilder.h"
#include "nsIXULTemplateQueryProcessor.h"

#include "nsISimpleEnumerator.h"
#include "nsCOMArray.h"
#include "nsIVariant.h"

#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "mozIStorageConnection.h"
#include "mozilla/Attributes.h"

class nsXULTemplateQueryProcessorStorage;

class nsXULTemplateResultSetStorage MOZ_FINAL : public nsISimpleEnumerator
{
private:

    nsCOMPtr<mozIStorageStatement> mStatement;

    nsCOMArray<nsIAtom> mColumnNames;

    ~nsXULTemplateResultSetStorage() {}

public:

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    explicit nsXULTemplateResultSetStorage(mozIStorageStatement* aStatement);

    int32_t GetColumnIndex(nsIAtom* aColumnName);

    void FillColumnValues(nsCOMArray<nsIVariant>& aArray);

};

class nsXULTemplateQueryProcessorStorage MOZ_FINAL : public nsIXULTemplateQueryProcessor
{
public:

    nsXULTemplateQueryProcessorStorage();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIXULTEMPLATEQUERYPROCESSOR

private:

    ~nsXULTemplateQueryProcessorStorage() {}

    nsCOMPtr<mozIStorageConnection> mStorageConnection;
    bool mGenerationStarted;
};

#endif 
