






































#ifndef __MOZSTORAGESTATEMENTJSHELPER_H__
#define __MOZSTORAGESTATEMENTJSHELPER_H__

#include "nsIXPCScriptable.h"

class Statement;

namespace mozilla {
namespace storage {

class StatementJSHelper : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

private:
  nsresult getRow(Statement *, JSContext *, JSObject *, jsval *);
  nsresult getParams(Statement *, JSContext *, JSObject *, jsval *);
};

} 
} 

#endif 
