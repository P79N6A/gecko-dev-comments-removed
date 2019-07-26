





#ifndef mozilla_dom_quota_usagerunnable_h__
#define mozilla_dom_quota_usagerunnable_h__

#include "mozilla/dom/quota/QuotaCommon.h"

#include "Utilities.h"

BEGIN_QUOTA_NAMESPACE

class UsageRunnable
{
public:
  UsageRunnable()
  : mCanceled(0), mDatabaseUsage(0), mFileUsage(0)
  { }

  virtual ~UsageRunnable()
  { }

  bool
  Canceled()
  {
    return mCanceled;
  }

  void
  AppendToDatabaseUsage(uint64_t aUsage)
  {
    IncrementUsage(&mDatabaseUsage, aUsage);
  }

  void
  AppendToFileUsage(uint64_t aUsage)
  {
    IncrementUsage(&mFileUsage, aUsage);
  }

  uint64_t
  DatabaseUsage()
  {
    return mDatabaseUsage;
  }

  uint64_t
  FileUsage()
  {
    return mFileUsage;
  }

  uint64_t
  TotalUsage()
  {
    uint64_t totalUsage = mDatabaseUsage;
    IncrementUsage(&totalUsage, mFileUsage);
    return totalUsage;
  }

  void
  ResetUsage()
  {
    mDatabaseUsage = 0;
    mFileUsage = 0;
  }

protected:
  int32_t mCanceled;

private:
  uint64_t mDatabaseUsage;
  uint64_t mFileUsage;
};

END_QUOTA_NAMESPACE

#endif 
