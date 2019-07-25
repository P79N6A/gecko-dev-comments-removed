






































#ifndef __NSCLIENTAUTHREMEMBER_H__
#define __NSCLIENTAUTHREMEMBER_H__

#include "mozilla/ReentrantMonitor.h"
#include "nsTHashtable.h"
#include "nsIObserver.h"
#include "nsIX509Cert.h"
#include "nsAutoPtr.h"
#include "nsNSSCertificate.h"
#include "nsString.h"
#include "nsWeakReference.h"

class nsClientAuthRemember
{
public:

  nsClientAuthRemember()
  {
  }
  
  nsClientAuthRemember(const nsClientAuthRemember &other)
  {
    this->operator=(other);
  }

  nsClientAuthRemember &operator=(const nsClientAuthRemember &other)
  {
    mAsciiHost = other.mAsciiHost;
    mFingerprint = other.mFingerprint;
    mDBKey = other.mDBKey;
    return *this;
  }

  nsCString mAsciiHost;
  nsCString mFingerprint;
  nsCString mDBKey;
};



class nsClientAuthRememberEntry : public PLDHashEntryHdr
{
  public:
    
    typedef const char* KeyType;
    typedef const char* KeyTypePointer;

    
    nsClientAuthRememberEntry(KeyTypePointer aHostWithCertUTF8)
    {
    }

    nsClientAuthRememberEntry(const nsClientAuthRememberEntry& toCopy)
    {
      mSettings = toCopy.mSettings;
    }

    ~nsClientAuthRememberEntry()
    {
    }

    KeyType GetKey() const
    {
      return HostWithCertPtr();
    }

    KeyTypePointer GetKeyPointer() const
    {
      return HostWithCertPtr();
    }

    PRBool KeyEquals(KeyTypePointer aKey) const
    {
      return !strcmp(HostWithCertPtr(), aKey);
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

    
    inline const nsCString &HostWithCert() const { return mHostWithCert; }

    inline KeyTypePointer HostWithCertPtr() const
    {
      return mHostWithCert.get();
    }

    nsClientAuthRemember mSettings;
    nsCString mHostWithCert;
};

class nsClientAuthRememberService : public nsIObserver,
                                    public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsClientAuthRememberService();
  ~nsClientAuthRememberService();

  nsresult Init();

  static void GetHostWithCert(const nsACString & aHostName, 
                              const nsACString & nickname, nsACString& _retval);

  nsresult RememberDecision(const nsACString & aHostName, 
                            CERTCertificate *aServerCert, CERTCertificate *aClientCert);
  nsresult HasRememberedDecision(const nsACString & aHostName, 
                                 CERTCertificate *aServerCert, 
                                 nsACString & aCertDBKey, PRBool *_retval);

  void ClearRememberedDecisions();

protected:
    mozilla::ReentrantMonitor monitor;
    nsTHashtable<nsClientAuthRememberEntry> mSettingsTable;

    void RemoveAllFromMemory();
    nsresult AddEntryToList(const nsACString &host, 
                            const nsACString &server_fingerprint,
                            const nsACString &db_key);
};

#endif
