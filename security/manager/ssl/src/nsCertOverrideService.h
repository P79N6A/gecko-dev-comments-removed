







































#ifndef __NSCERTOVERRIDESERVICE_H__
#define __NSCERTOVERRIDESERVICE_H__

#include "nsICertOverrideService.h"
#include "nsTHashtable.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "nsIFile.h"
#include "prmon.h"
#include "secoidt.h"
#include "nsWeakReference.h"

class nsCertOverride
{
public:

  enum OverrideBits { ob_None=0, ob_Untrusted=1, ob_Mismatch=2,
                      ob_Time_error=4 };

  nsCertOverride()
  :mOverrideBits(ob_None)
  {
  }

  nsCertOverride(const nsCertOverride &other)
  {
    this->operator=(other);
  }

  nsCertOverride &operator=(const nsCertOverride &other)
  {
    mHostWithPortUTF8 = other.mHostWithPortUTF8;
    mIsTemporary = other.mIsTemporary;
    mFingerprintAlgOID = other.mFingerprintAlgOID;
    mFingerprint = other.mFingerprint;
    mOverrideBits = other.mOverrideBits;
    mDBKey = other.mDBKey;
    return *this;
  }

  nsCString mHostWithPortUTF8;
  PRBool mIsTemporary; 
  nsCString mFingerprint;
  nsCString mFingerprintAlgOID;
  OverrideBits mOverrideBits;
  nsCString mDBKey;

  static void convertBitsToString(OverrideBits ob, nsACString &str);
  static void convertStringToBits(const nsACString &str, OverrideBits &ob);
};



class nsCertOverrideEntry : public PLDHashEntryHdr
{
  public:
    
    typedef const char* KeyType;
    typedef const char* KeyTypePointer;

    
    nsCertOverrideEntry(KeyTypePointer aHostWithPortUTF8)
    {
    }

    nsCertOverrideEntry(const nsCertOverrideEntry& toCopy)
    {
      mSettings = toCopy.mSettings;
    }

    ~nsCertOverrideEntry()
    {
    }

    KeyType GetKey() const
    {
      return HostWithPortPtr();
    }

    KeyTypePointer GetKeyPointer() const
    {
      return HostWithPortPtr();
    }

    PRBool KeyEquals(KeyTypePointer aKey) const
    {
      return !strcmp(HostWithPortPtr(), aKey);
    }

    static KeyTypePointer KeyToPointer(KeyType aKey)
    {
      return aKey;
    }

    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      
      
      return PL_DHashStringKey(nsnull, aKey);
    }

    enum { ALLOW_MEMMOVE = PR_FALSE };

    
    inline const nsCString &HostWithPort() const { return mSettings.mHostWithPortUTF8; }

    inline KeyTypePointer HostWithPortPtr() const
    {
      return mSettings.mHostWithPortUTF8.get();
    }

    nsCertOverride mSettings;
};

class nsCertOverrideService : public nsICertOverrideService
                            , public nsIObserver
                            , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICERTOVERRIDESERVICE
  NS_DECL_NSIOBSERVER

  nsCertOverrideService();
  ~nsCertOverrideService();

  nsresult Init();

  typedef void 
  (*PR_CALLBACK CertOverrideEnumerator)(const nsCertOverride &aSettings,
                                        void *aUserData);

  
  
  nsresult EnumerateCertOverrides(nsIX509Cert *aCert,
                                  CertOverrideEnumerator enumerator,
                                  void *aUserData);

protected:
    PRMonitor *monitor;
    nsCOMPtr<nsIFile> mSettingsFile;
    nsTHashtable<nsCertOverrideEntry> mSettingsTable;

    SECOidTag mOidTagForStoringNewHashes;
    nsCString mDottedOidForStoringNewHashes;

    void RemoveAllFromMemory();
    nsresult Read();
    nsresult Write();
    nsresult AddEntryToList(const nsACString &hostWithPortUTF8,
                            const PRBool aIsTemporary,
                            const nsACString &algo_oid, 
                            const nsACString &fingerprint,
                            nsCertOverride::OverrideBits ob,
                            const nsACString &dbKey);
};

#define NS_CERTOVERRIDE_CID { /* 67ba681d-5485-4fff-952c-2ee337ffdcd6 */ \
    0x67ba681d,                                                        \
    0x5485,                                                            \
    0x4fff,                                                            \
    {0x95, 0x2c, 0x2e, 0xe3, 0x37, 0xff, 0xdc, 0xd6}                   \
  }

#endif
