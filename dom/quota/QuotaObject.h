





#ifndef mozilla_dom_quota_quotaobject_h__
#define mozilla_dom_quota_quotaobject_h__

#include "mozilla/dom/quota/QuotaCommon.h"

#include "nsDataHashtable.h"

#include "PersistenceType.h"

BEGIN_QUOTA_NAMESPACE

class GroupInfo;
class GroupInfoPair;
class OriginInfo;
class QuotaManager;

class QuotaObject
{
  friend class OriginInfo;
  friend class QuotaManager;

public:
  void
  AddRef();

  void
  Release();

  void
  UpdateSize(int64_t aSize);

  bool
  MaybeAllocateMoreSpace(int64_t aOffset, int32_t aCount);

private:
  QuotaObject(OriginInfo* aOriginInfo, const nsAString& aPath, int64_t aSize)
  : mOriginInfo(aOriginInfo), mPath(aPath), mSize(aSize)
  {
    MOZ_COUNT_CTOR(QuotaObject);
  }

  ~QuotaObject()
  {
    MOZ_COUNT_DTOR(QuotaObject);
  }

  already_AddRefed<QuotaObject>
  LockedAddRef()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    ++mRefCnt;

    nsRefPtr<QuotaObject> result = dont_AddRef(this);
    return result.forget();
  }

  mozilla::ThreadSafeAutoRefCnt mRefCnt;

  OriginInfo* mOriginInfo;
  nsString mPath;
  int64_t mSize;
};

class OriginInfo MOZ_FINAL
{
  friend class GroupInfo;
  friend class QuotaManager;
  friend class QuotaObject;

public:
  OriginInfo(GroupInfo* aGroupInfo, const nsACString& aOrigin, bool aIsApp,
             uint64_t aLimit, uint64_t aUsage, int64_t aAccessTime)
  : mGroupInfo(aGroupInfo), mOrigin(aOrigin), mLimit(aLimit), mUsage(aUsage),
    mAccessTime(aAccessTime), mIsApp(aIsApp)
  {
    MOZ_COUNT_CTOR(OriginInfo);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(OriginInfo)

  int64_t
  AccessTime() const
  {
    return mAccessTime;
  }

  bool
  IsTreatedAsPersistent() const;

  bool
  IsTreatedAsTemporary() const;

private:
  
  ~OriginInfo()
  {
    MOZ_COUNT_DTOR(OriginInfo);
  }

  void
  LockedDecreaseUsage(int64_t aSize);

  void
  LockedUpdateAccessTime(int64_t aAccessTime)
  {
    AssertCurrentThreadOwnsQuotaMutex();

    mAccessTime = aAccessTime;
  }

  void
  LockedClearOriginInfos()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    mQuotaObjects.EnumerateRead(ClearOriginInfoCallback, nullptr);
  }

  static PLDHashOperator
  ClearOriginInfoCallback(const nsAString& aKey,
                          QuotaObject* aValue, void* aUserArg);

  nsDataHashtable<nsStringHashKey, QuotaObject*> mQuotaObjects;

  GroupInfo* mGroupInfo;
  const nsCString mOrigin;
  const uint64_t mLimit;
  uint64_t mUsage;
  int64_t mAccessTime;
  const bool mIsApp;
};

class OriginInfoLRUComparator
{
public:
  bool
  Equals(const OriginInfo* a, const OriginInfo* b) const
  {
    return
      a && b ? a->AccessTime() == b->AccessTime() : !a && !b ? true : false;
  }

  bool
  LessThan(const OriginInfo* a, const OriginInfo* b) const
  {
    return a && b ? a->AccessTime() < b->AccessTime() : b ? true : false;
  }
};

class GroupInfo MOZ_FINAL
{
  friend class GroupInfoPair;
  friend class OriginInfo;
  friend class QuotaManager;
  friend class QuotaObject;

public:
  GroupInfo(GroupInfoPair* aGroupInfoPair, PersistenceType aPersistenceType,
            const nsACString& aGroup)
  : mGroupInfoPair(aGroupInfoPair), mPersistenceType(aPersistenceType),
    mGroup(aGroup), mUsage(0)
  {
    MOZ_COUNT_CTOR(GroupInfo);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GroupInfo)

private:
  
  ~GroupInfo()
  {
    MOZ_COUNT_DTOR(GroupInfo);
  }

  already_AddRefed<OriginInfo>
  LockedGetOriginInfo(const nsACString& aOrigin);

  void
  LockedAddOriginInfo(OriginInfo* aOriginInfo);

  void
  LockedRemoveOriginInfo(const nsACString& aOrigin);

  void
  LockedRemoveOriginInfos();

  void
  LockedRemoveOriginInfosForPattern(const nsACString& aPattern);

  bool
  LockedHasOriginInfos()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    return !mOriginInfos.IsEmpty();
  }

  uint64_t
  LockedGetTemporaryUsage();

  void
  LockedGetTemporaryOriginInfos(nsTArray<OriginInfo*>* aOriginInfos);

  void
  LockedRemoveTemporaryOriginInfos();

  nsTArray<nsRefPtr<OriginInfo> > mOriginInfos;

  GroupInfoPair* mGroupInfoPair;
  PersistenceType mPersistenceType;
  nsCString mGroup;
  uint64_t mUsage;
};

class GroupInfoPair
{
  friend class QuotaManager;
  friend class QuotaObject;

public:
  GroupInfoPair()
  {
    MOZ_COUNT_CTOR(GroupInfoPair);
  }

  ~GroupInfoPair()
  {
    MOZ_COUNT_DTOR(GroupInfoPair);
  }

private:
  already_AddRefed<GroupInfo>
  LockedGetGroupInfo(PersistenceType aPersistenceType)
  {
    AssertCurrentThreadOwnsQuotaMutex();

    nsRefPtr<GroupInfo> groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    return groupInfo.forget();
  }

  void
  LockedSetGroupInfo(GroupInfo* aGroupInfo)
  {
    AssertCurrentThreadOwnsQuotaMutex();

    nsRefPtr<GroupInfo>& groupInfo =
      GetGroupInfoForPersistenceType(aGroupInfo->mPersistenceType);
    groupInfo = aGroupInfo;
  }

  void
  LockedClearGroupInfo(PersistenceType aPersistenceType)
  {
    AssertCurrentThreadOwnsQuotaMutex();

    nsRefPtr<GroupInfo>& groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    groupInfo = nullptr;
  }

  bool
  LockedHasGroupInfos()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    return mPersistentStorageGroupInfo || mTemporaryStorageGroupInfo;
  }

  nsRefPtr<GroupInfo>&
  GetGroupInfoForPersistenceType(PersistenceType aPersistenceType);

  nsRefPtr<GroupInfo> mPersistentStorageGroupInfo;
  nsRefPtr<GroupInfo> mTemporaryStorageGroupInfo;
};

END_QUOTA_NAMESPACE

#endif 
