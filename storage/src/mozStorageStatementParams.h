






































#ifndef _MOZSTORAGESTATEMENTPARAMS_H_
#define _MOZSTORAGESTATEMENTPARAMS_H_

#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"

class mozIStorageStatement;

namespace mozilla {
namespace storage {

class Statement;

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

  friend class Statement;
};

} 
} 

#endif 
