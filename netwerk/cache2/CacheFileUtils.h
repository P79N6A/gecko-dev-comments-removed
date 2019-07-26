



#ifndef CacheFileUtils__h__
#define CacheFileUtils__h__

#include "nsError.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"

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

} 
} 
} 

#endif
