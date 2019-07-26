





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

  if (groupInfo->IsForTemporaryStorage()) {
    quotaManager->mTemporaryStorageUsage -= mSize;
  }
  groupInfo->mUsage -= mSize;
  mOriginInfo->mUsage -= mSize;

  mSize = aSize;

  mOriginInfo->mUsage += mSize;
  groupInfo->mUsage += mSize;
  if (groupInfo->IsForTemporaryStorage()) {
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

  if (groupInfo->IsForPersistentStorage()) {
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

      quotaManager->LockedRemoveQuotaForOrigin(PERSISTENCE_TYPE_PERSISTENT,
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

  NS_ASSERTION(groupInfo->mPersistenceType == PERSISTENCE_TYPE_TEMPORARY,
               "Huh?");

  uint64_t delta = end - mSize;

  uint64_t newUsage = mOriginInfo->mUsage + delta;

  
  

  uint64_t newGroupUsage = groupInfo->mUsage + delta;

  
  
  if (newGroupUsage > quotaManager->GetGroupLimit()) {
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
        quotaManager->DeleteTemporaryFilesForOrigin(originInfos[i]->mOrigin);
      }
    }

    

    NS_ASSERTION(mOriginInfo, "How come?!");

    nsTArray<nsCString> origins;
    for (uint32_t i = 0; i < originInfos.Length(); i++) {
      OriginInfo* originInfo = originInfos[i];

      NS_ASSERTION(originInfo != mOriginInfo, "Deleted itself!");

      nsCString group = originInfo->mGroupInfo->mGroup;
      nsCString origin = originInfo->mOrigin;
      quotaManager->LockedRemoveQuotaForOrigin(PERSISTENCE_TYPE_TEMPORARY,
                                               group, origin);

#ifdef DEBUG
      originInfos[i] = nullptr;
#endif

      origins.AppendElement(origin);
    }

    
    

    delta = end - mSize;

    newUsage = mOriginInfo->mUsage + delta;

    newGroupUsage = groupInfo->mUsage + delta;

    if (newGroupUsage > quotaManager->GetGroupLimit()) {
      
      

      
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

void
OriginInfo::LockedDecreaseUsage(int64_t aSize)
{
  AssertCurrentThreadOwnsQuotaMutex();

  mUsage -= aSize;

  mGroupInfo->mUsage -= aSize;

  if (mGroupInfo->IsForTemporaryStorage()) {
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

  if (IsForTemporaryStorage()) {
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

      if (IsForTemporaryStorage()) {
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
    mUsage -= mOriginInfos[index - 1]->mUsage;

    if (IsForTemporaryStorage()) {
      QuotaManager* quotaManager = QuotaManager::Get();
      NS_ASSERTION(quotaManager, "Shouldn't be null!");

      quotaManager->mTemporaryStorageUsage -= mOriginInfos[index - 1]->mUsage;
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

      if (IsForTemporaryStorage()) {
        QuotaManager* quotaManager = QuotaManager::Get();
        NS_ASSERTION(quotaManager, "Shouldn't be null!");

        quotaManager->mTemporaryStorageUsage -= mOriginInfos[index - 1]->mUsage;
      }

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
