






































#ifndef _MOZSTORAGESTATEMENTPARAMS_H_
#define _MOZSTORAGESTATEMENTPARAMS_H_

#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"

#include "jsapi.h"
#include "jsdate.h"

class mozIStorageStatement;
class mozStorageStatement;

namespace mozilla {
namespace storage {

class StatementParams : public mozIStorageStatementParams
                      , public nsIXPCScriptable
{
public:
  StatementParams(mozIStorageStatement *aStatement);

  
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTPARAMS
  NS_DECL_NSIXPCSCRIPTABLE

protected:
  mozIStorageStatement *mStatement;
  PRUint32 mParamCount;

  friend class ::mozStorageStatement;
};

static
PRBool
JSValStorageStatementBinder(JSContext *aCtx,
                            mozIStorageStatement *aStatement,
                            int aIdx,
                            jsval aValue)
{
  if (JSVAL_IS_INT(aValue)) {
    int v = JSVAL_TO_INT(aValue);
    (void)aStatement->BindInt32Parameter(aIdx, v);
  }
  else if (JSVAL_IS_DOUBLE(aValue)) {
    double d = *JSVAL_TO_DOUBLE(aValue);
    (void)aStatement->BindDoubleParameter(aIdx, d);
  }
  else if (JSVAL_IS_STRING(aValue)) {
    JSString *str = JSVAL_TO_STRING(aValue);
    nsDependentString value(
      reinterpret_cast<PRUnichar *>(::JS_GetStringChars(str)),
      ::JS_GetStringLength(str)
    );
    (void)aStatement->BindStringParameter(aIdx, value);
  }
  else if (JSVAL_IS_BOOLEAN(aValue)) {
    (void)aStatement->BindInt32Parameter(aIdx, (aValue == JSVAL_TRUE) ? 1 : 0);
  }
  else if (JSVAL_IS_NULL(aValue)) {
    (void)aStatement->BindNullParameter(aIdx);
  }
  else if (JSVAL_IS_OBJECT(aValue)) {
    JSObject *obj = JSVAL_TO_OBJECT(aValue);
    
    if (::js_DateIsValid (aCtx, obj)) {
      double msecd = ::js_DateGetMsecSinceEpoch(aCtx, obj);
      msecd *= 1000.0;
      PRInt64 msec;
      LL_D2L(msec, msecd);

      (void)aStatement->BindInt64Parameter(aIdx, msec);
    }
    else {
      return PR_FALSE;
    }
  }
  else {
    return PR_FALSE;
  }

  return PR_TRUE;
}

} 
} 

#endif 
