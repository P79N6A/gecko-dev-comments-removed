



#ifndef CacheIndexContextIterator__h__
#define CacheIndexContextIterator__h__

#include "CacheIndexIterator.h"

class nsILoadContextInfo;

namespace mozilla {
namespace net {

class CacheIndexContextIterator : public CacheIndexIterator
{
public:
  CacheIndexContextIterator(CacheIndex *aIndex, bool aAddNew,
                            nsILoadContextInfo *aInfo);
  virtual ~CacheIndexContextIterator();

private:
  virtual void AddRecord(CacheIndexRecord *aRecord);
  virtual void AddRecords(const nsTArray<CacheIndexRecord *> &aRecords);

  nsCOMPtr<nsILoadContextInfo> mInfo;
};

} 
} 

#endif
