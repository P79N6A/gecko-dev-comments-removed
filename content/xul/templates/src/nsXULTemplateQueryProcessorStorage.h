




































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

class nsXULTemplateQueryProcessorStorage;

class nsXULTemplateResultSetStorage : public nsISimpleEnumerator
{
private:

    nsCOMPtr<mozIStorageStatement> mStatement;

    nsCOMArray<nsIAtom> mColumnNames;

public:

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    nsXULTemplateResultSetStorage(mozIStorageStatement* aStatement);

    PRInt32 GetColumnIndex(nsIAtom* aColumnName);

    void FillColumnValues(nsCOMArray<nsIVariant>& aArray);

};

class nsXULTemplateQueryProcessorStorage : public nsIXULTemplateQueryProcessor
{
public:

    nsXULTemplateQueryProcessorStorage();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIXULTEMPLATEQUERYPROCESSOR

private:

    nsCOMPtr<mozIStorageConnection> mStorageConnection;
    bool mGenerationStarted;
};

#endif 
