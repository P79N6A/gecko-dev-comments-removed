



#ifndef CacheIndexIterator__h__
#define CacheIndexIterator__h__

#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/SHA1.h"

namespace mozilla {
namespace net {

class CacheIndex;
struct CacheIndexRecord;

class CacheIndexIterator
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CacheIndexIterator)

  CacheIndexIterator(CacheIndex *aIndex, bool aAddNew);

protected:
  virtual ~CacheIndexIterator();

public:
  
  
  
  nsresult GetNextHash(SHA1Sum::Hash *aHash);

  
  
  nsresult Close();

protected:
  friend class CacheIndex;

  nsresult CloseInternal(nsresult aStatus);

  bool ShouldBeNewAdded() { return mAddNew; }
  virtual void AddRecord(CacheIndexRecord *aRecord);
  virtual void AddRecords(const nsTArray<CacheIndexRecord *> &aRecords);
  bool RemoveRecord(CacheIndexRecord *aRecord);
  bool ReplaceRecord(CacheIndexRecord *aOldRecord,
                     CacheIndexRecord *aNewRecord);

  nsresult                     mStatus;
  nsRefPtr<CacheIndex>         mIndex;
  nsTArray<CacheIndexRecord *> mRecords;
  bool                         mAddNew;
};

} 
} 

#endif
