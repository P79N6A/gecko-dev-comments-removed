






































#ifndef _MOZSTORAGESTATEMENTROW_H_
#define _MOZSTORAGESTATEMENTROW_H_

#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"

class mozStorageStatement;


namespace mozilla {
namespace storage {

class StatementRow : public mozIStorageStatementRow
                   , public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTROW
  NS_DECL_NSIXPCSCRIPTABLE

  StatementRow(mozStorageStatement *aStatement);
protected:

  mozStorageStatement *mStatement;

  friend class ::mozStorageStatement;
};

} 
} 

#endif 
