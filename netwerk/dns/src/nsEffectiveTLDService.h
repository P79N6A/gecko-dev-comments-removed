





































#include "nsIEffectiveTLDService.h"

#include "nsTHashtable.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsIIDNService;
class nsIFile;


class nsDomainEntry : public PLDHashEntryHdr
{
public:
  
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  nsDomainEntry(const char* aDomain);

  nsDomainEntry(const nsDomainEntry& toCopy)
  {
    
    
    NS_NOTREACHED("nsDomainEntry copy constructor is forbidden!");
  }

  ~nsDomainEntry()
  {
  }

  KeyType GetKey() const
  {
    return mDomain;
  }

  PRBool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mDomain, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    
    return PL_DHashStringKey(nsnull, aKey);
  }

  enum { ALLOW_MEMMOVE = PR_TRUE };

  PRPackedBool& IsNormal()    { return mIsNormal; }
  PRPackedBool& IsException() { return mIsException; }
  PRPackedBool& IsWild()      { return mIsWild; }

private:
  const char   *mDomain;
  PRPackedBool  mIsNormal;
  PRPackedBool  mIsException;
  PRPackedBool  mIsWild;
};

class nsEffectiveTLDService : public nsIEffectiveTLDService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEFFECTIVETLDSERVICE

  nsEffectiveTLDService();
  nsresult Init();

private:
  nsresult NormalizeHostname(nsCString &aHostname);
  nsresult AddEffectiveTLDEntry(nsCString &aDomainName);
  nsresult LoadEffectiveTLDFiles();
  nsresult LoadOneEffectiveTLDFile(nsCOMPtr<nsIFile>& effTLDFile);

  virtual ~nsEffectiveTLDService();

  nsTHashtable<nsDomainEntry> mHash;
  nsCOMPtr<nsIIDNService>     mIDNService;
};
