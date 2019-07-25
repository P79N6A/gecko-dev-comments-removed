





































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
  nsDataHashtable<nsCStringHashKey, PRUint64> mScopesVersion;

  static PRUint64 NextGlobalVersion();
  PRUint64 CachedScopeVersion(DOMStorageImpl* aStorage);

  void MarkScopeDirty(DOMStorageImpl* aStorage);
  void MarkAllScopesDirty();

private:
  static PRUint64 sGlobalVersion;
};

#endif 
