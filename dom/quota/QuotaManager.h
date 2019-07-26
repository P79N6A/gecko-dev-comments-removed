





#ifndef mozilla_dom_quota_quotamanager_h__
#define mozilla_dom_quota_quotamanager_h__

#include "QuotaCommon.h"

#include "mozilla/Mutex.h"
#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsThreadUtils.h"

BEGIN_QUOTA_NAMESPACE

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
  { }

  virtual ~QuotaObject()
  { }

  nsAutoRefCnt mRefCnt;

  OriginInfo* mOriginInfo;
  nsString mPath;
  int64_t mSize;
};

class OriginInfo
{
  friend class QuotaManager;
  friend class QuotaObject;

public:
  OriginInfo(const nsACString& aOrigin, int64_t aLimit, int64_t aUsage)
  : mOrigin(aOrigin), mLimit(aLimit), mUsage(aUsage)
  {
    mQuotaObjects.Init();
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(OriginInfo)

private:
  void
#ifdef DEBUG
  LockedClearOriginInfos();
#else
  LockedClearOriginInfos()
  {
    mQuotaObjects.EnumerateRead(ClearOriginInfoCallback, nullptr);
  }
#endif

  static PLDHashOperator
  ClearOriginInfoCallback(const nsAString& aKey,
                          QuotaObject* aValue, void* aUserArg);

  nsDataHashtable<nsStringHashKey, QuotaObject*> mQuotaObjects;

  nsCString mOrigin;
  int64_t mLimit;
  int64_t mUsage;
};

class QuotaManager
{
  friend class nsAutoPtr<QuotaManager>;
  friend class OriginInfo;
  friend class QuotaObject;

public:
  
  static QuotaManager*
  GetOrCreate();

  
  static QuotaManager*
  Get();

  void
  InitQuotaForOrigin(const nsACString& aOrigin,
                     int64_t aLimit,
                     int64_t aUsage);

  void
  DecreaseUsageForOrigin(const nsACString& aOrigin,
                         int64_t aSize);

  void
  RemoveQuotaForPattern(const nsACString& aPattern);

  already_AddRefed<QuotaObject>
  GetQuotaObject(const nsACString& aOrigin,
                 nsIFile* aFile);

  already_AddRefed<QuotaObject>
  GetQuotaObject(const nsACString& aOrigin,
                 const nsAString& aPath);

private:
  QuotaManager()
  : mQuotaMutex("QuotaManager.mQuotaMutex")
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    mOriginInfos.Init();
  }

  virtual ~QuotaManager()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  }

  mozilla::Mutex mQuotaMutex;

  nsRefPtrHashtable<nsCStringHashKey, OriginInfo> mOriginInfos;
};

END_QUOTA_NAMESPACE

#endif 
