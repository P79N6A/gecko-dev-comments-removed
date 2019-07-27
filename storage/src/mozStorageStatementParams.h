





#ifndef MOZSTORAGESTATEMENTPARAMS_H
#define MOZSTORAGESTATEMENTPARAMS_H

#include "mozIStorageStatementParams.h"
#include "nsIXPCScriptable.h"
#include "mozilla/Attributes.h"

class mozIStorageStatement;

namespace mozilla {
namespace storage {

class StatementParams final : public mozIStorageStatementParams
                            , public nsIXPCScriptable
{
public:
  explicit StatementParams(mozIStorageStatement *aStatement);

  
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTPARAMS
  NS_DECL_NSIXPCSCRIPTABLE

protected:
  ~StatementParams() {}

  mozIStorageStatement *mStatement;
  uint32_t mParamCount;

  friend class StatementParamsHolder;
  friend class StatementRowHolder;
};

} 
} 

#endif 
