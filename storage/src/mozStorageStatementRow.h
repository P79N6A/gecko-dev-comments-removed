





#ifndef MOZSTORAGESTATEMENTROW_H
#define MOZSTORAGESTATEMENTROW_H

#include "mozIStorageStatementRow.h"
#include "nsIXPCScriptable.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace storage {

class Statement;

class StatementRow MOZ_FINAL : public mozIStorageStatementRow
                             , public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTROW
  NS_DECL_NSIXPCSCRIPTABLE

  explicit StatementRow(Statement *aStatement);
protected:

  ~StatementRow() {}

  Statement *mStatement;

  friend class Statement;
};

} 
} 

#endif 
