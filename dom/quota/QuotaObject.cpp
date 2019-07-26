





#include "QuotaObject.h"

#include "QuotaManager.h"

USING_QUOTA_NAMESPACE

void
QuotaObject::AddRef()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  if (!quotaManager) {
    NS_ERROR("Null quota manager, this shouldn't happen, possible leak!");

    NS_AtomicIncrementRefcnt(mRefCnt);

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

    nsrefcnt count = NS_AtomicDecrementRefcnt(mRefCnt);
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

  if (mOriginInfo) {
    mOriginInfo->mUsage -= mSize;
    mSize = aSize;
    mOriginInfo->mUsage += mSize;
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

  int64_t newUsage = mOriginInfo->mUsage - mSize + end;
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

    nsCString origin = mOriginInfo->mOrigin;

    mOriginInfo->LockedClearOriginInfos();
    NS_ASSERTION(!mOriginInfo,
                 "Should have cleared in LockedClearOriginInfos!");

    quotaManager->mOriginInfos.Remove(origin);

    
    
    NS_ASSERTION(mSize < end, "This shouldn't happen!");

    mSize = end;

    return true;
  }

  mOriginInfo->mUsage = newUsage;
  mSize = end;

  return true;
}

#ifdef DEBUG
void
OriginInfo::LockedClearOriginInfos()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  quotaManager->mQuotaMutex.AssertCurrentThreadOwns();

  mQuotaObjects.EnumerateRead(ClearOriginInfoCallback, nullptr);
}
#endif


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
