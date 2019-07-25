







































#include "nsIEffectiveTLDService.h"

#include "nsTHashtable.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsIIDNService;


struct ETLDEntry {
  const char* domain;
  bool exception;
  bool wild;
};



class nsDomainEntry : public PLDHashEntryHdr
{
public:
  
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  nsDomainEntry(KeyTypePointer aEntry)
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
    return mData->domain;
  }

  bool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mData->domain, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    
    return PL_DHashStringKey(nsnull, aKey);
  }

  enum { ALLOW_MEMMOVE = true };

  void SetData(const ETLDEntry* entry) { mData = entry; }

  bool IsNormal() { return mData->wild || !mData->exception; }
  bool IsException() { return mData->exception; }
  bool IsWild() { return mData->wild; }

private:
  const ETLDEntry* mData;
};

class nsEffectiveTLDService : public nsIEffectiveTLDService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEFFECTIVETLDSERVICE

  nsEffectiveTLDService() { }
  nsresult Init();

private:
  nsresult GetBaseDomainInternal(nsCString &aHostname, PRUint32 aAdditionalParts, nsACString &aBaseDomain);
  nsresult NormalizeHostname(nsCString &aHostname);
  ~nsEffectiveTLDService() { }

  nsTHashtable<nsDomainEntry> mHash;
  nsCOMPtr<nsIIDNService>     mIDNService;
};
