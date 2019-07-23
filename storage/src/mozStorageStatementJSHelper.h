






































#ifndef __MOZSTORAGESTATEMENTJSHELPER_H__
#define __MOZSTORAGESTATEMENTJSHELPER_H__

#include "nsIXPCScriptable.h"

class mozStorageStatement;

namespace mozilla {
namespace storage {

class StatementJSHelper : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

private:
  nsresult getRow(mozStorageStatement *, JSContext *, JSObject *, jsval *);
  nsresult getParams(mozStorageStatement *, JSContext *, JSObject *, jsval *);
};

} 
} 

#endif 
