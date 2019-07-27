





#include "QuotaObject.h"

#include "QuotaManager.h"
#include "Utilities.h"

USING_QUOTA_NAMESPACE

void
QuotaObject::AddRef()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  if (!quotaManager) {
    NS_ERROR("Null quota manager, this shouldn't happen, possible leak!");

    ++mRefCnt;

    return;
  }

  MutexAutoLock lock(quotaManager->mQuotaMutex);

  ++mRefCnt;
}

void
QuotaObject::Release()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  if (!quotaManager) {
    NS_ERROR("Null quota manager, this shouldn't happen, possible leak!");

    nsrefcnt count = --mRefCnt;
    if (count == 0) {
      mRefCnt = 1;
      delete this;
    }

    return;
  }

  {
    MutexAutoLock lock(quotaManager->mQuotaMutex);

    --mRefCnt;

    if (mRefCnt > 0) {
      return;
    }

    if (mOriginInfo) {
      mOriginInfo->mQuotaObjects.Remove(mPath);
    }
  }

  delete this;
}

void
QuotaObject::UpdateSize(int64_t aSize)
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  MutexAutoLock lock(quotaManager->mQuotaMutex);

  if (!mOriginInfo) {
    return;
  }

  GroupInfo* groupInfo = mOriginInfo->mGroupInfo;

  if (mOriginInfo->IsTreatedAsTemporary()) {
    quotaManager->mTemporaryStorageUsage -= mSize;
  }
  groupInfo->mUsage -= mSize;
  mOriginInfo->mUsage -= mSize;

  mSize = aSize;

  mOriginInfo->mUsage += mSize;
  groupInfo->mUsage += mSize;
  if (mOriginInfo->IsTreatedAsTemporary()) {
    quotaManager->mTemporaryStorageUsage += mSize;
  }
}

bool
QuotaObject::MaybeAllocateMoreSpace(int64_t aOffset, int32_t aCount)
{
  int64_t end = aOffset + aCount;

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  MutexAutoLock lock(quotaManager->mQuotaMutex);

  if (mSize >= end || !mOriginInfo) {
    return true;
  }

  GroupInfo* groupInfo = mOriginInfo->mGroupInfo;

  if (mOriginInfo->IsTreatedAsPersistent()) {
    uint64_t newUsage = mOriginInfo->mUsage - mSize + end;

    if (newUsage > mOriginInfo->mLimit) {
      
      
      
      if (!quotaManager->LockedQuotaIsLifted()) {
        return false;
      }

      
      
      if (!mOriginInfo) {
        
        if (end > mSize) {
          mSize = end;
        }

        return true;
      }

      nsCString group = mOriginInfo->mGroupInfo->mGroup;
      nsCString origin = mOriginInfo->mOrigin;

      mOriginInfo->LockedClearOriginInfos();
      NS_ASSERTION(!mOriginInfo,
                   "Should have cleared in LockedClearOriginInfos!");

      quotaManager->LockedRemoveQuotaForOrigin(groupInfo->mPersistenceType,
                                               group, origin);

      
      
      NS_ASSERTION(mSize < end, "This shouldn't happen!");

      mSize = end;

      return true;
    }

    mOriginInfo->mUsage = newUsage;

    groupInfo->mUsage = groupInfo->mUsage - mSize + end;

    mSize = end;

    return true;
  }

  nsRefPtr<GroupInfo> complementaryGroupInfo =
    groupInfo->mGroupInfoPair->LockedGetGroupInfo(
      ComplementaryPersistenceType(groupInfo->mPersistenceType));

  uint64_t delta = end - mSize;

  uint64_t newUsage = mOriginInfo->mUsage + delta;

  
  

  uint64_t newGroupUsage = groupInfo->mUsage + delta;

  uint64_t groupUsage = groupInfo->LockedGetTemporaryUsage();
  if (complementaryGroupInfo) {
    groupUsage += complementaryGroupInfo->LockedGetTemporaryUsage();
  }

  
  
  if (groupUsage + delta > quotaManager->GetGroupLimit()) {
    return false;
  }

  uint64_t newTemporaryStorageUsage = quotaManager->mTemporaryStorageUsage +
                                      delta;

  if (newTemporaryStorageUsage > quotaManager->mTemporaryStorageLimit) {
    

    nsAutoTArray<OriginInfo*, 10> originInfos;
    uint64_t sizeToBeFreed =
      quotaManager->LockedCollectOriginsForEviction(delta, originInfos);

    if (!sizeToBeFreed) {
      return false;
    }

    NS_ASSERTION(sizeToBeFreed >= delta, "Huh?");

    {
      MutexAutoUnlock autoUnlock(quotaManager->mQuotaMutex);

      for (uint32_t i = 0; i < originInfos.Length(); i++) {
        OriginInfo* originInfo = originInfos[i];

        quotaManager->DeleteFilesForOrigin(
                                       originInfo->mGroupInfo->mPersistenceType,
                                       originInfo->mOrigin);
      }
    }

    

    NS_ASSERTION(mOriginInfo, "How come?!");

    nsTArray<OriginParams> origins;
    for (uint32_t i = 0; i < originInfos.Length(); i++) {
      OriginInfo* originInfo = originInfos[i];

      NS_ASSERTION(originInfo != mOriginInfo, "Deleted itself!");

      PersistenceType persistenceType =
        originInfo->mGroupInfo->mPersistenceType;
      nsCString group = originInfo->mGroupInfo->mGroup;
      nsCString origin = originInfo->mOrigin;
      quotaManager->LockedRemoveQuotaForOrigin(persistenceType, group, origin);

#ifdef DEBUG
      originInfos[i] = nullptr;
#endif

      origins.AppendElement(OriginParams(persistenceType, origin));
    }

    
    

    delta = end - mSize;

    newUsage = mOriginInfo->mUsage + delta;

    newGroupUsage = groupInfo->mUsage + delta;

    groupUsage = groupInfo->LockedGetTemporaryUsage();
    if (complementaryGroupInfo) {
      groupUsage += complementaryGroupInfo->LockedGetTemporaryUsage();
    }

    if (groupUsage + delta > quotaManager->GetGroupLimit()) {
      
      

      
      MutexAutoUnlock autoUnlock(quotaManager->mQuotaMutex);

      quotaManager->FinalizeOriginEviction(origins);

      return false;
    }

    newTemporaryStorageUsage = quotaManager->mTemporaryStorageUsage + delta;

    NS_ASSERTION(newTemporaryStorageUsage <=
                 quotaManager->mTemporaryStorageLimit, "How come?!");

    
    

    mOriginInfo->mUsage = newUsage;
    groupInfo->mUsage = newGroupUsage;
    quotaManager->mTemporaryStorageUsage = newTemporaryStorageUsage;;

    
    
    NS_ASSERTION(mSize < end, "This shouldn't happen!");
    mSize = end;

    
    
    MutexAutoUnlock autoUnlock(quotaManager->mQuotaMutex);

    quotaManager->FinalizeOriginEviction(origins);

    return true;
  }

  mOriginInfo->mUsage = newUsage;
  groupInfo->mUsage = newGroupUsage;
  quotaManager->mTemporaryStorageUsage = newTemporaryStorageUsage;

  mSize = end;

  return true;
}

bool
OriginInfo::IsTreatedAsPersistent() const
{
  return QuotaManager::IsTreatedAsPersistent(mGroupInfo->mPersistenceType,
                                             mOrigin);
}

bool
OriginInfo::IsTreatedAsTemporary() const
{
  return QuotaManager::IsTreatedAsTemporary(mGroupInfo->mPersistenceType,
                                            mOrigin);
}

void
OriginInfo::LockedDecreaseUsage(int64_t aSize)
{
  AssertCurrentThreadOwnsQuotaMutex();

  mUsage -= aSize;

  mGroupInfo->mUsage -= aSize;

  if (IsTreatedAsTemporary()) {
    QuotaManager* quotaManager = QuotaManager::Get();
    NS_ASSERTION(quotaManager, "Shouldn't be null!");

    quotaManager->mTemporaryStorageUsage -= aSize;
  }
}


PLDHashOperator
OriginInfo::ClearOriginInfoCallback(const nsAString& aKey,
                                    QuotaObject* aValue,
                                    void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  aValue->mOriginInfo = nullptr;

  return PL_DHASH_NEXT;
}

already_AddRefed<OriginInfo>
GroupInfo::LockedGetOriginInfo(const nsACString& aOrigin)
{
  AssertCurrentThreadOwnsQuotaMutex();

  for (uint32_t index = 0; index < mOriginInfos.Length(); index++) {
    nsRefPtr<OriginInfo>& originInfo = mOriginInfos[index];

    if (originInfo->mOrigin == aOrigin) {
      nsRefPtr<OriginInfo> result = originInfo;
      return result.forget();
    }
  }

  return nullptr;
}

void
GroupInfo::LockedAddOriginInfo(OriginInfo* aOriginInfo)
{
  AssertCurrentThreadOwnsQuotaMutex();

  NS_ASSERTION(!mOriginInfos.Contains(aOriginInfo),
               "Replacing an existing entry!");
  mOriginInfos.AppendElement(aOriginInfo);

  mUsage += aOriginInfo->mUsage;

  if (aOriginInfo->IsTreatedAsTemporary()) {
    QuotaManager* quotaManager = QuotaManager::Get();
    NS_ASSERTION(quotaManager, "Shouldn't be null!");

    quotaManager->mTemporaryStorageUsage += aOriginInfo->mUsage;
  }
}

void
GroupInfo::LockedRemoveOriginInfo(const nsACString& aOrigin)
{
  AssertCurrentThreadOwnsQuotaMutex();

  for (uint32_t index = 0; index < mOriginInfos.Length(); index++) {
    if (mOriginInfos[index]->mOrigin == aOrigin) {
      mUsage -= mOriginInfos[index]->mUsage;

      if (mOriginInfos[index]->IsTreatedAsTemporary()) {
        QuotaManager* quotaManager = QuotaManager::Get();
        NS_ASSERTION(quotaManager, "Shouldn't be null!");

        quotaManager->mTemporaryStorageUsage -= mOriginInfos[index]->mUsage;
      }

      mOriginInfos.RemoveElementAt(index);

      return;
    }
  }
}

void
GroupInfo::LockedRemoveOriginInfos()
{
  AssertCurrentThreadOwnsQuotaMutex();

  for (uint32_t index = mOriginInfos.Length(); index > 0; index--) {
    OriginInfo* originInfo = mOriginInfos[index - 1];

    mUsage -= originInfo->mUsage;

    if (originInfo->IsTreatedAsTemporary()) {
      QuotaManager* quotaManager = QuotaManager::Get();
      NS_ASSERTION(quotaManager, "Shouldn't be null!");

      quotaManager->mTemporaryStorageUsage -= originInfo->mUsage;
    }

    mOriginInfos.RemoveElementAt(index - 1);
  }
}

void
GroupInfo::LockedRemoveOriginInfosForPattern(const nsACString& aPattern)
{
  AssertCurrentThreadOwnsQuotaMutex();

  for (uint32_t index = mOriginInfos.Length(); index > 0; index--) {
    if (PatternMatchesOrigin(aPattern, mOriginInfos[index - 1]->mOrigin)) {
      mUsage -= mOriginInfos[index - 1]->mUsage;

      if (mOriginInfos[index - 1]->IsTreatedAsTemporary()) {
        QuotaManager* quotaManager = QuotaManager::Get();
        NS_ASSERTION(quotaManager, "Shouldn't be null!");

        quotaManager->mTemporaryStorageUsage -= mOriginInfos[index - 1]->mUsage;
      }

      mOriginInfos.RemoveElementAt(index - 1);
    }
  }
}

uint64_t
GroupInfo::LockedGetTemporaryUsage()
{
  uint64_t usage = 0;

  for (uint32_t count = mOriginInfos.Length(), index = 0;
       index < count;
       index++) {
    nsRefPtr<OriginInfo>& originInfo = mOriginInfos[index];

    if (originInfo->IsTreatedAsTemporary()) {
      usage += originInfo->mUsage;
    }
  }

  return usage;
}

void
GroupInfo::LockedGetTemporaryOriginInfos(nsTArray<OriginInfo*>* aOriginInfos)
{
  AssertCurrentThreadOwnsQuotaMutex();

  for (uint32_t count = mOriginInfos.Length(), index = 0;
       index < count;
       index++) {
    nsRefPtr<OriginInfo>& originInfo = mOriginInfos[index];

    if (originInfo->IsTreatedAsTemporary()) {
      aOriginInfos->AppendElement(originInfo);
    }
  }
}

void
GroupInfo::LockedRemoveTemporaryOriginInfos()
{
  AssertCurrentThreadOwnsQuotaMutex();

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  for (uint32_t index = mOriginInfos.Length(); index > 0; index--) {
    OriginInfo* originInfo = mOriginInfos[index - 1];
    if (originInfo->IsTreatedAsTemporary()) {
      MOZ_ASSERT(mUsage >= originInfo->mUsage);
      mUsage -= originInfo->mUsage;

      MOZ_ASSERT(quotaManager->mTemporaryStorageUsage >= originInfo->mUsage);
      quotaManager->mTemporaryStorageUsage -= originInfo->mUsage;

      mOriginInfos.RemoveElementAt(index - 1);
    }
  }
}

nsRefPtr<GroupInfo>&
GroupInfoPair::GetGroupInfoForPersistenceType(PersistenceType aPersistenceType)
{
  switch (aPersistenceType) {
    case PERSISTENCE_TYPE_PERSISTENT:
      return mPersistentStorageGroupInfo;
    case PERSISTENCE_TYPE_TEMPORARY:
      return mTemporaryStorageGroupInfo;

    case PERSISTENCE_TYPE_INVALID:
    default:
      MOZ_CRASH("Bad persistence type value!");
      return mPersistentStorageGroupInfo;
  }
}
