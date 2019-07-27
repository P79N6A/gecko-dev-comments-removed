





#ifndef mozilla_dom_cache_Connection_h
#define mozilla_dom_cache_Connection_h

#include "mozIStorageConnection.h"

namespace mozilla {
namespace dom {
namespace cache {

class Connection final : public mozIStorageConnection
{
public:
  explicit Connection(mozIStorageConnection* aBase);

private:
  ~Connection();

  nsCOMPtr<mozIStorageConnection> mBase;
  bool mClosed;

  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEASYNCCONNECTION
  NS_DECL_MOZISTORAGECONNECTION
};

} 
} 
} 

#endif
