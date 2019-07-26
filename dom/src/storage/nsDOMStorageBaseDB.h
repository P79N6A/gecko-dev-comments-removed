




#ifndef nsDOMStorageBaseDB_h___
#define nsDOMStorageBaseDB_h___

#include "nscore.h"
#include "nsDataHashtable.h"

class DOMStorageImpl;

class nsDOMStorageBaseDB
{
public:
  static void Init();

  nsDOMStorageBaseDB();
  virtual ~nsDOMStorageBaseDB() {}

  








  void MarkScopeCached(DOMStorageImpl* aStorage);

  



  bool IsScopeDirty(DOMStorageImpl* aStorage);

  int32_t GetQuota() {
    return gQuotaLimit * 1024;
  }

protected:
  nsDataHashtable<nsCStringHashKey, uint64_t> mScopesVersion;

  static uint64_t NextGlobalVersion();
  uint64_t CachedScopeVersion(DOMStorageImpl* aStorage);

  void MarkScopeDirty(DOMStorageImpl* aStorage);
  void MarkAllScopesDirty();

private:
  static uint64_t sGlobalVersion;

  static int32_t gQuotaLimit;
};

#endif 
