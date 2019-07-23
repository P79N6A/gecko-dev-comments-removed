






































#ifndef _MOZSTORAGESTATEMENTROW_H_
#define _MOZSTORAGESTATEMENTROW_H_

#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"

namespace mozilla {
namespace storage {

class Statement;

class StatementRow : public mozIStorageStatementRow
                   , public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTROW
  NS_DECL_NSIXPCSCRIPTABLE

  StatementRow(Statement *aStatement);
protected:

  Statement *mStatement;

  friend class Statement;
};

} 
} 

#endif 
