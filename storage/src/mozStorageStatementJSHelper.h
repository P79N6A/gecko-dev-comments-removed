





#ifndef MOZSTORAGESTATEMENTJSHELPER_H
#define MOZSTORAGESTATEMENTJSHELPER_H

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
  nsresult getRow(Statement *, JSContext *, JSObject *, JS::Value *);
  nsresult getParams(Statement *, JSContext *, JSObject *, JS::Value *);
};

} 
} 

#endif 
