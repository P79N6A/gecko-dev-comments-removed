



#ifndef CacheFileUtils__h__
#define CacheFileUtils__h__

#include "nsError.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/TimeStamp.h"

class nsILoadContextInfo;
class nsACString;

namespace mozilla {
namespace net {
namespace CacheFileUtils {

already_AddRefed<nsILoadContextInfo>
ParseKey(const nsCSubstring &aKey,
         nsCSubstring *aIdEnhance = nullptr,
         nsCSubstring *aURISpec = nullptr);

void
AppendKeyPrefix(nsILoadContextInfo *aInfo, nsACString &_retval);

void
AppendTagWithValue(nsACString & aTarget, char const aTag, nsCSubstring const & aValue);

nsresult
KeyMatchesLoadContextInfo(const nsACString &aKey,
                          nsILoadContextInfo *aInfo,
                          bool *_retval);

class ValidityPair {
public:
  ValidityPair(uint32_t aOffset, uint32_t aLen);

  ValidityPair& operator=(const ValidityPair& aOther);

  
  
  bool CanBeMerged(const ValidityPair& aOther) const;

  
  
  bool IsInOrFollows(uint32_t aOffset) const;

  
  
  
  bool LessThan(const ValidityPair& aOther) const;

  
  void Merge(const ValidityPair& aOther);

  uint32_t Offset() const { return mOffset; }
  uint32_t Len() const    { return mLen; }

private:
  uint32_t mOffset;
  uint32_t mLen;
};

class ValidityMap {
public:
  
  void Log() const;

  
  uint32_t Length() const;

  
  
  void AddPair(uint32_t aOffset, uint32_t aLen);

  
  void Clear();

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

  ValidityPair& operator[](uint32_t aIdx);

private:
  nsTArray<ValidityPair> mMap;
};


class DetailedCacheHitTelemetry {
public:
  enum ERecType {
    HIT  = 0,
    MISS = 1
  };

  static void AddRecord(ERecType aType, TimeStamp aLoadStart);

private:
  class HitRate {
  public:
    HitRate();

    void     AddRecord(ERecType aType);
    
    
    uint32_t GetHitRateBucket(uint32_t aNumOfBuckets) const;
    uint32_t Count();
    void     Reset();

  private:
    uint32_t mHitCnt;
    uint32_t mMissCnt;
  };

  
  
  static const uint32_t kRangeSize = 5000;
  static const uint32_t kNumOfRanges = 20;

  
  
  static const uint32_t kTotalSamplesReportLimit = 1000;

  
  
  
  static const uint32_t kHitRateSamplesReportLimit = 500;

  
  
  
  
  
  static const uint32_t kHitRateBuckets = 20;

  
  static StaticMutex sLock;

  
  static uint32_t sRecordCnt;
 
  
  static HitRate sHRStats[kNumOfRanges];
};

} 
} 
} 

#endif
