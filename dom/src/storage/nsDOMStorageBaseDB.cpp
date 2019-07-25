





































#include "nsDOMStorageBaseDB.h"
#include "nsDOMStorage.h"

PRUint64 nsDOMStorageBaseDB::sGlobalVersion = 1;

nsDOMStorageBaseDB::nsDOMStorageBaseDB()
{
  mScopesVersion.Init(8);
}



void
nsDOMStorageBaseDB::MarkScopeCached(DOMStorageImpl* aStorage)
{
  aStorage->SetCachedVersion(CachedScopeVersion(aStorage));
}

bool
nsDOMStorageBaseDB::IsScopeDirty(DOMStorageImpl* aStorage)
{
  return !aStorage->CachedVersion() ||
         (aStorage->CachedVersion() != CachedScopeVersion(aStorage));
}




PRUint64
nsDOMStorageBaseDB::NextGlobalVersion()
{
  sGlobalVersion++;
  if (sGlobalVersion == 0) 
    sGlobalVersion = 1;
  return sGlobalVersion;
}

PRUint64
nsDOMStorageBaseDB::CachedScopeVersion(DOMStorageImpl* aStorage)
{
  PRUint64 currentVersion;
  if (mScopesVersion.Get(aStorage->GetScopeDBKey(), &currentVersion))
    return currentVersion;

  mScopesVersion.Put(aStorage->GetScopeDBKey(), sGlobalVersion);
  return sGlobalVersion;
}

void
nsDOMStorageBaseDB::MarkScopeDirty(DOMStorageImpl* aStorage)
{
  PRUint64 nextVersion = NextGlobalVersion();
  mScopesVersion.Put(aStorage->GetScopeDBKey(), nextVersion);

  
  
  aStorage->SetCachedVersion(nextVersion);
}

void
nsDOMStorageBaseDB::MarkAllScopesDirty()
{
  mScopesVersion.Clear();
  NextGlobalVersion();
}
