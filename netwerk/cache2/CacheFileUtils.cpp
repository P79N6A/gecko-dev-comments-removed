



#include "CacheLog.h"
#include "CacheFileUtils.h"
#include "LoadContextInfo.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include <algorithm>


namespace mozilla {
namespace net {
namespace CacheFileUtils {

namespace { 




class KeyParser
{
public:
  KeyParser(nsACString::const_iterator aCaret, nsACString::const_iterator aEnd)
    : caret(aCaret)
    , end(aEnd)
    
    , appId(nsILoadContextInfo::NO_APP_ID)
    , isPrivate(false)
    , isInBrowser(false)
    , isAnonymous(false)
    
    , cacheKey(aEnd)
    , lastTag(0)
  {
  }

private:
  
  nsACString::const_iterator caret;
  
  nsACString::const_iterator const end;

  
  uint32_t appId;
  bool isPrivate;
  bool isInBrowser;
  bool isAnonymous;
  nsCString idEnhance;
  
  nsACString::const_iterator cacheKey;

  
  char lastTag;

  bool ParseTags()
  {
    
    if (caret == end)
      return true;

    
    char const tag = *caret++;
    
    if (!(lastTag < tag || tag == ':'))
      return false;

    lastTag = tag;

    switch (tag) {
    case ':':
      
      
      cacheKey = caret;
      caret = end;
      return true;
    case 'p':
      isPrivate = true;
      break;
    case 'b':
      isInBrowser = true;
      break;
    case 'a':
      isAnonymous = true;
      break;
    case 'i': {
      nsAutoCString appIdString;
      if (!ParseValue(&appIdString))
        return false;

      nsresult rv;
      int64_t appId64 = appIdString.ToInteger64(&rv);
      if (NS_FAILED(rv))
        return false; 
      if (appId64 < 0 || appId64 > PR_UINT32_MAX)
        return false; 
      appId = static_cast<uint32_t>(appId64);

      break;
    }
    case '~':
      if (!ParseValue(&idEnhance))
        return false;
      break;
    default:
      if (!ParseValue()) 
        return false;
      break;
    }

    
    return ParseNextTagOrEnd();
  }

  bool ParseNextTagOrEnd()
  {
    
    if (caret == end || *caret++ != ',')
      return false;

    
    return ParseTags();
  }

  bool ParseValue(nsACString * result = nullptr)
  {
    
    if (caret == end)
      return false;

    
    nsACString::const_iterator val = caret;
    nsACString::const_iterator comma = end;
    bool escape = false;
    while (caret != end) {
      nsACString::const_iterator at = caret;
      ++caret; 

      if (*at == ',') {
        if (comma != end) {
          
          comma = end;
          escape = true;
        } else {
          comma = at;
        }
        continue;
      }

      if (comma != end) {
        
        break;
      }
    }

    
    
    

    caret = comma;
    if (result) {
      if (escape) {
        
        nsAutoCString _result(Substring(val, caret));
        _result.ReplaceSubstring(NS_LITERAL_CSTRING(",,"), NS_LITERAL_CSTRING(","));
        result->Assign(_result);
      } else {
        result->Assign(Substring(val, caret));
      }
    }

    return caret != end;
  }

public:
  already_AddRefed<LoadContextInfo> Parse()
  {
    nsRefPtr<LoadContextInfo> info;
    if (ParseTags())
      info = GetLoadContextInfo(isPrivate, appId, isInBrowser, isAnonymous);

    return info.forget();
  }

  void URISpec(nsACString &result)
  {
    
    result.Assign(Substring(cacheKey, end));
  }

  void IdEnhance(nsACString &result)
  {
    result.Assign(idEnhance);
  }
};

} 

already_AddRefed<nsILoadContextInfo>
ParseKey(const nsCSubstring &aKey,
         nsCSubstring *aIdEnhance,
         nsCSubstring *aURISpec)
{
  nsACString::const_iterator caret, end;
  aKey.BeginReading(caret);
  aKey.EndReading(end);

  KeyParser parser(caret, end);
  nsRefPtr<LoadContextInfo> info = parser.Parse();

  if (info) {
    if (aIdEnhance)
      parser.IdEnhance(*aIdEnhance);
    if (aURISpec)
      parser.URISpec(*aURISpec);
  }

  return info.forget();
}

void
AppendKeyPrefix(nsILoadContextInfo* aInfo, nsACString &_retval)
{
  







  if (aInfo->IsAnonymous()) {
    _retval.AppendLiteral("a,");
  }

  if (aInfo->IsInBrowserElement()) {
    _retval.AppendLiteral("b,");
  }

  if (aInfo->AppId() != nsILoadContextInfo::NO_APP_ID) {
    _retval.Append('i');
    _retval.AppendInt(aInfo->AppId());
    _retval.Append(',');
  }

  if (aInfo->IsPrivate()) {
    _retval.AppendLiteral("p,");
  }
}

void
AppendTagWithValue(nsACString & aTarget, char const aTag, nsCSubstring const & aValue)
{
  aTarget.Append(aTag);

  
  
  if (!aValue.IsEmpty()) {
    if (aValue.FindChar(',') == kNotFound) {
      
      aTarget.Append(aValue);
    } else {
      nsAutoCString escapedValue(aValue);
      escapedValue.ReplaceSubstring(
        NS_LITERAL_CSTRING(","), NS_LITERAL_CSTRING(",,"));
      aTarget.Append(escapedValue);
    }
  }

  aTarget.Append(',');
}

nsresult
KeyMatchesLoadContextInfo(const nsACString &aKey, nsILoadContextInfo *aInfo,
                          bool *_retval)
{
  nsCOMPtr<nsILoadContextInfo> info = ParseKey(aKey);

  if (!info) {
    return NS_ERROR_FAILURE;
  }

  *_retval = info->Equals(aInfo);
  return NS_OK;
}

ValidityPair::ValidityPair(uint32_t aOffset, uint32_t aLen)
  : mOffset(aOffset), mLen(aLen)
{}

ValidityPair&
ValidityPair::operator=(const ValidityPair& aOther)
{
  mOffset = aOther.mOffset;
  mLen = aOther.mLen;
  return *this;
}

bool
ValidityPair::CanBeMerged(const ValidityPair& aOther) const
{
  
  
  
  return IsInOrFollows(aOther.mOffset) || aOther.IsInOrFollows(mOffset);
}

bool
ValidityPair::IsInOrFollows(uint32_t aOffset) const
{
  return mOffset <= aOffset && mOffset + mLen >= aOffset;
}

bool
ValidityPair::LessThan(const ValidityPair& aOther) const
{
  if (mOffset < aOther.mOffset) {
    return true;
  }

  if (mOffset == aOther.mOffset && mLen < aOther.mLen) {
    return true;
  }

  return false;
}

void
ValidityPair::Merge(const ValidityPair& aOther)
{
  MOZ_ASSERT(CanBeMerged(aOther));

  uint32_t offset = std::min(mOffset, aOther.mOffset);
  uint32_t end = std::max(mOffset + mLen, aOther.mOffset + aOther.mLen);

  mOffset = offset;
  mLen = end - offset;
}

void
ValidityMap::Log() const
{
  LOG(("ValidityMap::Log() - number of pairs: %u", mMap.Length()));
  for (uint32_t i=0; i<mMap.Length(); i++) {
    LOG(("    (%u, %u)", mMap[i].Offset() + 0, mMap[i].Len() + 0));
  }
}

uint32_t
ValidityMap::Length() const
{
  return mMap.Length();
}

void
ValidityMap::AddPair(uint32_t aOffset, uint32_t aLen)
{
  ValidityPair pair(aOffset, aLen);

  if (mMap.Length() == 0) {
    mMap.AppendElement(pair);
    return;
  }

  
  
  uint32_t pos = 0;
  for (pos = mMap.Length(); pos > 0; ) {
    --pos;

    if (mMap[pos].LessThan(pair)) {
      
      if (mMap[pos].CanBeMerged(pair)) {
        
        mMap[pos].Merge(pair);
      } else {
        
        ++pos;
        if (pos == mMap.Length()) {
          mMap.AppendElement(pair);
        } else {
          mMap.InsertElementAt(pos, pair);
        }
      }

      break;
    }

    if (pos == 0) {
      
      mMap.InsertElementAt(0, pair);
    }
  }

  
  
  while (pos + 1 < mMap.Length()) {
    if (mMap[pos].CanBeMerged(mMap[pos + 1])) {
      mMap[pos].Merge(mMap[pos + 1]);
      mMap.RemoveElementAt(pos + 1);
    } else {
      break;
    }
  }
}

void
ValidityMap::Clear()
{
  mMap.Clear();
}

size_t
ValidityMap::SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  return mMap.SizeOfExcludingThis(mallocSizeOf);
}

ValidityPair&
ValidityMap::operator[](uint32_t aIdx)
{
  return mMap.ElementAt(aIdx);
}

StaticMutex DetailedCacheHitTelemetry::sLock;
uint32_t DetailedCacheHitTelemetry::sRecordCnt = 0;
DetailedCacheHitTelemetry::HitRate DetailedCacheHitTelemetry::sHRStats[kNumOfRanges];

DetailedCacheHitTelemetry::HitRate::HitRate()
{
  Reset();
}

void
DetailedCacheHitTelemetry::HitRate::AddRecord(ERecType aType)
{
  if (aType == HIT) {
    ++mHitCnt;
  } else {
    ++mMissCnt;
  }
}

uint32_t
DetailedCacheHitTelemetry::HitRate::GetHitRateBucket(uint32_t aNumOfBuckets) const
{
  uint32_t bucketIdx = (aNumOfBuckets * mHitCnt) / (mHitCnt + mMissCnt);
  if (bucketIdx == aNumOfBuckets) { 
    --bucketIdx;
  }

  return bucketIdx;
}

uint32_t
DetailedCacheHitTelemetry::HitRate::Count()
{
  return mHitCnt + mMissCnt;
}

void
DetailedCacheHitTelemetry::HitRate::Reset()
{
  mHitCnt = 0;
  mMissCnt = 0;
}


void
DetailedCacheHitTelemetry::AddRecord(ERecType aType, TimeStamp aLoadStart)
{
  bool isUpToDate = false;
  CacheIndex::IsUpToDate(&isUpToDate);
  if (!isUpToDate) {
    
    return;
  }

  uint32_t entryCount;
  nsresult rv = CacheIndex::GetEntryFileCount(&entryCount);
  if (NS_FAILED(rv)) {
    return;
  }

  uint32_t rangeIdx = entryCount / kRangeSize;
  if (rangeIdx >= kNumOfRanges) { 
    rangeIdx = kNumOfRanges - 1;
  }

  uint32_t hitMissValue = 2 * rangeIdx; 
  if (aType == MISS) { 
    ++hitMissValue;
  }

  StaticMutexAutoLock lock(sLock);

  if (aType == MISS) {
    mozilla::Telemetry::AccumulateTimeDelta(
      mozilla::Telemetry::NETWORK_CACHE_V2_MISS_TIME_MS,
      aLoadStart);
  } else {
    mozilla::Telemetry::AccumulateTimeDelta(
      mozilla::Telemetry::NETWORK_CACHE_V2_HIT_TIME_MS,
      aLoadStart);
  }

  Telemetry::Accumulate(Telemetry::NETWORK_CACHE_HIT_MISS_STAT_PER_CACHE_SIZE,
                        hitMissValue);

  sHRStats[rangeIdx].AddRecord(aType);
  ++sRecordCnt;

  if (sRecordCnt < kTotalSamplesReportLimit) {
    return;
  }

  sRecordCnt = 0;

  for (uint32_t i = 0; i < kNumOfRanges; ++i) {
    if (sHRStats[i].Count() >= kHitRateSamplesReportLimit) {
      
      
      
      
      uint32_t bucketOffset = sHRStats[i].GetHitRateBucket(kHitRateBuckets) *
                              kNumOfRanges;

      Telemetry::Accumulate(Telemetry::NETWORK_CACHE_HIT_RATE_PER_CACHE_SIZE,
                            bucketOffset + i);
      sHRStats[i].Reset();
    }
  }
}

} 
} 
} 
