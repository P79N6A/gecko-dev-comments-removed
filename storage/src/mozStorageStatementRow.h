





































#ifndef _MOZSTORAGESTATEMENTROW_H_
#define _MOZSTORAGESTATEMENTROW_H_

#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"
#include "mozStorageStatement.h"
#include "nsString.h"
#include "nsVoidArray.h"

class mozStorageStatementRow : public mozIStorageStatementRow,
                               public nsIXPCScriptable
{
public:
    mozStorageStatementRow(mozStorageStatement *aStatement);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_MOZISTORAGESTATEMENTROW

    
    NS_DECL_NSIXPCSCRIPTABLE
protected:

    mozStorageStatement *mStatement;

    friend class mozStorageStatement;
};

#endif 
