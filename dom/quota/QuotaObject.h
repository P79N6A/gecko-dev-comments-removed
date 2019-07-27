





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

class OriginInfo final
{
  friend class GroupInfo;
  friend class QuotaManager;
  friend class QuotaObject;

public:
  OriginInfo(GroupInfo* aGroupInfo, const nsACString& aOrigin, bool aIsApp,
             uint64_t aUsage, int64_t aAccessTime)
  : mGroupInfo(aGroupInfo), mOrigin(aOrigin), mUsage(aUsage),
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

private:
  
  ~OriginInfo()
  {
    MOZ_COUNT_DTOR(OriginInfo);

    MOZ_ASSERT(!mQuotaObjects.Count());
  }

  void
  LockedDecreaseUsage(int64_t aSize);

  void
  LockedUpdateAccessTime(int64_t aAccessTime)
  {
    AssertCurrentThreadOwnsQuotaMutex();

    mAccessTime = aAccessTime;
  }

  nsDataHashtable<nsStringHashKey, QuotaObject*> mQuotaObjects;

  GroupInfo* mGroupInfo;
  const nsCString mOrigin;
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

class GroupInfo final
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
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

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

  bool
  LockedHasOriginInfos()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    return !mOriginInfos.IsEmpty();
  }

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
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

    nsRefPtr<GroupInfo> groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    return groupInfo.forget();
  }

  void
  LockedSetGroupInfo(PersistenceType aPersistenceType, GroupInfo* aGroupInfo)
  {
    AssertCurrentThreadOwnsQuotaMutex();
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

    nsRefPtr<GroupInfo>& groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    groupInfo = aGroupInfo;
  }

  void
  LockedClearGroupInfo(PersistenceType aPersistenceType)
  {
    AssertCurrentThreadOwnsQuotaMutex();
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

    nsRefPtr<GroupInfo>& groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    groupInfo = nullptr;
  }

  bool
  LockedHasGroupInfos()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    return mTemporaryStorageGroupInfo || mDefaultStorageGroupInfo;
  }

  nsRefPtr<GroupInfo>&
  GetGroupInfoForPersistenceType(PersistenceType aPersistenceType);

  nsRefPtr<GroupInfo> mTemporaryStorageGroupInfo;
  nsRefPtr<GroupInfo> mDefaultStorageGroupInfo;
};

END_QUOTA_NAMESPACE

#endif 
