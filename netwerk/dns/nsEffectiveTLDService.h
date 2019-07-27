




#ifndef EffectiveTLDService_h
#define EffectiveTLDService_h

#include "nsIEffectiveTLDService.h"

#include "nsIMemoryReporter.h"
#include "nsTHashtable.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

class nsIIDNService;

#define ETLD_ENTRY_N_INDEX_BITS 30


struct ETLDEntry {
  uint32_t strtab_index : ETLD_ENTRY_N_INDEX_BITS;
  uint32_t exception : 1;
  uint32_t wild : 1;
};



class nsDomainEntry : public PLDHashEntryHdr
{
  friend class nsEffectiveTLDService;
public:
  
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  explicit nsDomainEntry(KeyTypePointer aEntry)
  {
  }

  nsDomainEntry(const nsDomainEntry& toCopy)
  {
    
    
    NS_NOTREACHED("nsDomainEntry copy constructor is forbidden!");
  }

  ~nsDomainEntry()
  {
  }

  KeyType GetKey() const
  {
    return GetEffectiveTLDName(mData->strtab_index);
  }

  bool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(GetKey(), aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    
    return PL_DHashStringKey(nullptr, aKey);
  }

  enum { ALLOW_MEMMOVE = true };

  void SetData(const ETLDEntry* entry) { mData = entry; }

  bool IsNormal() { return mData->wild || !mData->exception; }
  bool IsException() { return mData->exception; }
  bool IsWild() { return mData->wild; }

  static const char *GetEffectiveTLDName(size_t idx)
  {
    return strings.strtab + idx;
  }

private:
  const ETLDEntry* mData;
#define ETLD_STR_NUM_1(line) str##line
#define ETLD_STR_NUM(line) ETLD_STR_NUM_1(line)
  struct etld_string_list {
#define ETLD_ENTRY(name, ex, wild) char ETLD_STR_NUM(__LINE__)[sizeof(name)];
#include "etld_data.inc"
#undef ETLD_ENTRY
  };
  static const union etld_strings {
    struct etld_string_list list;
    char strtab[1];
  } strings;
  static const ETLDEntry entries[];
  void FuncForStaticAsserts(void);
#undef ETLD_STR_NUM
#undef ETLD_STR_NUM1
};

class nsEffectiveTLDService MOZ_FINAL
  : public nsIEffectiveTLDService
  , public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEFFECTIVETLDSERVICE
  NS_DECL_NSIMEMORYREPORTER

  nsEffectiveTLDService();
  nsresult Init();

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

private:
  nsresult GetBaseDomainInternal(nsCString &aHostname, int32_t aAdditionalParts, nsACString &aBaseDomain);
  nsresult NormalizeHostname(nsCString &aHostname);
  ~nsEffectiveTLDService();

  nsTHashtable<nsDomainEntry> mHash;
  nsCOMPtr<nsIIDNService>     mIDNService;
};

#endif 
