





































#ifndef _MOZSTORAGESTATEMENTPARAMS_H_
#define _MOZSTORAGESTATEMENTPARAMS_H_

#include "mozIStorageStatement.h"
#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"

#include "jsapi.h"
#include "jsdate.h"

class mozStorageStatement;

class mozStorageStatementParams : public mozIStorageStatementParams,
                                  public nsIXPCScriptable
{
public:
    mozStorageStatementParams(mozIStorageStatement *aStatement);

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGESTATEMENTPARAMS
    NS_DECL_NSIXPCSCRIPTABLE

protected:
    mozIStorageStatement *mStatement;
    PRUint32 mParamCount;

    friend class mozStorageStatement;
};

static PRBool
JSValStorageStatementBinder (JSContext *cx,
                             mozIStorageStatement *aStatement,
                             int aIdx,
                             jsval val)
{
    if (JSVAL_IS_INT(val)) {
        int v = JSVAL_TO_INT(val);
        (void)aStatement->BindInt32Parameter(aIdx, v);
    } else if (JSVAL_IS_DOUBLE(val)) {
        double d = *JSVAL_TO_DOUBLE(val);
        (void)aStatement->BindDoubleParameter(aIdx, d);
    } else if (JSVAL_IS_STRING(val)) {
        JSString *str = JSVAL_TO_STRING(val);
        (void)aStatement->BindStringParameter(aIdx, nsDependentString(reinterpret_cast<PRUnichar*>(JS_GetStringChars(str)), JS_GetStringLength(str)));
    } else if (JSVAL_IS_BOOLEAN(val)) {
        (void)aStatement->BindInt32Parameter(aIdx, (val == JSVAL_TRUE) ? 1 : 0);
    } else if (JSVAL_IS_NULL(val)) {
        (void)aStatement->BindNullParameter(aIdx);
    } else if (JSVAL_IS_OBJECT(val)) {
        JSObject *obj = JSVAL_TO_OBJECT(val);
        
        if (js_DateIsValid (cx, obj)) {
            double msecd = js_DateGetMsecSinceEpoch(cx, obj);
            msecd *= 1000.0;
            PRInt64 msec;
            LL_D2L(msec, msecd);

            (void)aStatement->BindInt64Parameter(aIdx, msec);
        } else {
            return PR_FALSE;
        }
    } else {
        return PR_FALSE;
    }

    return PR_TRUE;
}

#endif 
