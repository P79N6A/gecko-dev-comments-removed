





































#ifndef nsCookie_h__
#define nsCookie_h__

#include "nsICookie.h"
#include "nsICookie2.h"
#include "nsString.h"












class nsCookie : public nsICookie2
{
  
  public:
    NS_DECL_ISUPPORTS_INHERITED
  protected:
    NS_DECL_OWNINGTHREAD

  public:
    
    NS_DECL_NSICOOKIE
    NS_DECL_NSICOOKIE2

  private:
    
    nsCookie(const char     *aName,
             const char     *aValue,
             const char     *aHost,
             const char     *aPath,
             const char     *aEnd,
             PRInt64         aExpiry,
             PRInt64         aCreationID,
             PRBool          aIsSession,
             PRBool          aIsSecure,
             PRBool          aIsHttpOnly,
             nsCookieStatus  aStatus,
             nsCookiePolicy  aPolicy)
     : mNext(nsnull)
     , mName(aName)
     , mValue(aValue)
     , mHost(aHost)
     , mPath(aPath)
     , mEnd(aEnd)
     , mExpiry(aExpiry)
     , mCreationID(aCreationID)
     , mRefCnt(0)
     , mIsSession(aIsSession != PR_FALSE)
     , mIsSecure(aIsSecure != PR_FALSE)
     , mIsHttpOnly(aIsHttpOnly != PR_FALSE)
     , mStatus(aStatus)
     , mPolicy(aPolicy)
    {
    }

  public:
    
    
    static nsCookie * Create(const nsACString &aName,
                             const nsACString &aValue,
                             const nsACString &aHost,
                             const nsACString &aPath,
                             PRInt64           aExpiry,
                             PRInt64           aCreationID,
                             PRBool            aIsSession,
                             PRBool            aIsSecure,
                             PRBool            aIsHttpOnly,
                             nsCookieStatus    aStatus,
                             nsCookiePolicy    aPolicy);

    virtual ~nsCookie() {}

    
    inline const nsDependentCString Name()  const { return nsDependentCString(mName, mValue - 1); }
    inline const nsDependentCString Value() const { return nsDependentCString(mValue, mHost - 1); }
    inline const nsDependentCString Host()  const { return nsDependentCString(mHost, mPath - 1); }
    inline const nsDependentCString RawHost() const { return nsDependentCString(IsDomain() ? mHost + 1 : mHost, mPath - 1); }
    inline const nsDependentCString Path()  const { return nsDependentCString(mPath, mEnd); }
    inline PRInt64 Expiry()                 const { return mExpiry; }
    inline PRInt64 CreationID()             const { return mCreationID; }
    
    inline PRInt64 CreationTime()           const { return mCreationID / PR_USEC_PER_SEC; }
    inline PRBool IsSession()               const { return mIsSession; }
    inline PRBool IsDomain()                const { return *mHost == '.'; }
    inline PRBool IsSecure()                const { return mIsSecure; }
    inline PRBool IsHttpOnly()              const { return mIsHttpOnly; }
    inline nsCookieStatus Status()          const { return mStatus; }
    inline nsCookiePolicy Policy()          const { return mPolicy; }

    
    inline void SetExpiry(PRInt64 aExpiry)        { mExpiry = aExpiry; }
    inline void SetIsSession(PRBool aIsSession)   { mIsSession = aIsSession; }
    
    
    inline void SetCreationID(PRInt64 aID)        { mCreationID = aID; }

    
    inline nsCookie*& Next() { return mNext; }

  protected:
    
    
    
    
    

    nsCookie   *mNext;
    const char *mName;
    const char *mValue;
    const char *mHost;
    const char *mPath;
    const char *mEnd;
    PRInt64     mExpiry;
    
    
    PRInt64     mCreationID;
    PRUint32    mRefCnt    : 16;
    PRUint32    mIsSession : 1;
    PRUint32    mIsSecure  : 1;
    PRUint32    mIsHttpOnly: 1;
    PRUint32    mStatus    : 3;
    PRUint32    mPolicy    : 3;
};

#endif 
