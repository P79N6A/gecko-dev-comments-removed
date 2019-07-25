







































#ifndef mozilla_dom_indexeddb_indexeddatabase_h__
#define mozilla_dom_indexeddb_indexeddatabase_h__

#include "nsIProgrammingLanguage.h"

#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsDOMError.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

#define DB_SCHEMA_VERSION 8

#define BEGIN_INDEXEDDB_NAMESPACE \
  namespace mozilla { namespace dom { namespace indexedDB {

#define END_INDEXEDDB_NAMESPACE \
  } /* namespace indexedDB */ } /* namepsace dom */ } /* namespace mozilla */

#define USING_INDEXEDDB_NAMESPACE \
  using namespace mozilla::dom::indexedDB;

BEGIN_INDEXEDDB_NAMESPACE

inline
void
AppendConditionClause(const nsACString& aColumnName,
                      const nsACString& aArgName,
                      bool aLessThan,
                      bool aEquals,
                      nsACString& aResult)
{
  aResult += NS_LITERAL_CSTRING(" AND ") + aColumnName +
             NS_LITERAL_CSTRING(" ");

  if (aLessThan) {
    aResult.AppendLiteral("<");
  }
  else {
    aResult.AppendLiteral(">");
  }

  if (aEquals) {
    aResult.AppendLiteral("=");
  }

  aResult += NS_LITERAL_CSTRING(" :") + aArgName;
}

END_INDEXEDDB_NAMESPACE

#endif 
