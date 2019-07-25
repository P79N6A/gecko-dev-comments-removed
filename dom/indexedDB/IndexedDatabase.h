







































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

#define BEGIN_INDEXEDDB_NAMESPACE \
  namespace mozilla { namespace dom { namespace indexedDB {

#define END_INDEXEDDB_NAMESPACE \
  } /* namespace indexedDB */ } /* namepsace dom */ } /* namespace mozilla */

#define USING_INDEXEDDB_NAMESPACE \
  using namespace mozilla::dom::indexedDB;

#endif 
