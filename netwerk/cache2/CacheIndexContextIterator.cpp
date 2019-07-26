



#include "CacheLog.h"
#include "CacheIndexContextIterator.h"
#include "CacheIndex.h"
#include "nsString.h"


namespace mozilla {
namespace net {

CacheIndexContextIterator::CacheIndexContextIterator(CacheIndex *aIndex,
                                                     bool aAddNew,
                                                     nsILoadContextInfo *aInfo)
  : CacheIndexIterator(aIndex, aAddNew)
  , mInfo(aInfo)
{
}

CacheIndexContextIterator::~CacheIndexContextIterator()
{
}

void
CacheIndexContextIterator::AddRecord(CacheIndexRecord *aRecord)
{
  if (CacheIndexEntry::RecordMatchesLoadContextInfo(aRecord, mInfo)) {
    CacheIndexIterator::AddRecord(aRecord);
  }
}

void
CacheIndexContextIterator::AddRecords(
  const nsTArray<CacheIndexRecord *> &aRecords)
{
  
  for (uint32_t i = 0; i < aRecords.Length(); ++i) {
    AddRecord(aRecords[i]);
  }
}

} 
} 
