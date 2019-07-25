




#ifndef nsDOMStorageBaseDB_h___
#define nsDOMStorageBaseDB_h___

#include "nscore.h"
#include "nsDataHashtable.h"

class DOMStorageImpl;

class nsDOMStorageBaseDB
{
public:
  nsDOMStorageBaseDB();
  virtual ~nsDOMStorageBaseDB() {}

  








  void MarkScopeCached(DOMStorageImpl* aStorage);

  



  bool IsScopeDirty(DOMStorageImpl* aStorage);

protected:
  nsDataHashtable<nsCStringHashKey, uint64_t> mScopesVersion;

  static uint64_t NextGlobalVersion();
  uint64_t CachedScopeVersion(DOMStorageImpl* aStorage);

  void MarkScopeDirty(DOMStorageImpl* aStorage);
  void MarkAllScopesDirty();

private:
  static uint64_t sGlobalVersion;
};

#endif 
