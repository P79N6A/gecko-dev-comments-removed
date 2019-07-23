





































#ifndef _MOZSTORAGEVALUEARRAY_H_
#define _MOZSTORAGEVALUEARRAY_H_

#include "nsCOMPtr.h"

#include "mozIStorageValueArray.h"
#include "mozIStorageDataSet.h"

#include "nsIArray.h"

#include <sqlite3.h>

class nsIArray;

class mozStorageStatementRowValueArray : public mozIStorageValueArray
{
public:
    mozStorageStatementRowValueArray (sqlite3_stmt *aSqliteStmt);
    ~mozStorageStatementRowValueArray();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGEVALUEARRAY

private:
    sqlite3_stmt *mSqliteStatement;
    PRUint32 mNumEntries;
};

class mozStorageArgvValueArray : public mozIStorageValueArray
{
public:
    mozStorageArgvValueArray (PRInt32 aArgc, sqlite3_value **aArgv);
    ~mozStorageArgvValueArray();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGEVALUEARRAY

private:
    PRUint32 mArgc;
    sqlite3_value **mArgv;
};

class mozStorageDataSet : public mozIStorageDataSet
{
public:
    mozStorageDataSet (nsIArray *aRows);
    ~mozStorageDataSet();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGEDATASET

protected:
    nsCOMPtr<nsIArray> mRows;
};

#endif 
