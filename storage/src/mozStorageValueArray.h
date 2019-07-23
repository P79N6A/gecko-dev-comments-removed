





































#ifndef _MOZSTORAGEVALUEARRAY_H_
#define _MOZSTORAGEVALUEARRAY_H_

#include "mozIStorageValueArray.h"

#include <sqlite3.h>

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

#endif 
