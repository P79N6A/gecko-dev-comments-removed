





































#ifndef nsCookie_h__
#define nsCookie_h__

#include "nsICookie.h"
#include "nsICookie2.h"
#include "nsString.h"












class nsCookie : public nsICookie2
{
  public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSICOOKIE
    NS_DECL_NSICOOKIE2

  private:
    
    nsCookie(const char     *aName,
             const char     *aValue,
             const char     *aHost,
             const char     *aPath,
             const char     *aEnd,
             PRInt64         aExpiry,
             PRInt64         aLastAccessed,
             PRInt64         aCreationTime,
             bool            aIsSession,
             bool            aIsSecure,
             bool            aIsHttpOnly)
     : mName(aName)
     , mValue(aValue)
     , mHost(aHost)
     , mPath(aPath)
     , mEnd(aEnd)
     , mExpiry(aExpiry)
     , mLastAccessed(aLastAccessed)
     , mCreationTime(aCreationTime)
     , mIsSession(aIsSession != PR_FALSE)
     , mIsSecure(aIsSecure != PR_FALSE)
     , mIsHttpOnly(aIsHttpOnly != PR_FALSE)
    {
    }

  public:
    
    
    static PRInt64 GenerateUniqueCreationTime(PRInt64 aCreationTime);

    
    
    static nsCookie * Create(const nsACString &aName,
                             const nsACString &aValue,
                             const nsACString &aHost,
                             const nsACString &aPath,
                             PRInt64           aExpiry,
                             PRInt64           aLastAccessed,
                             PRInt64           aCreationTime,
                             bool              aIsSession,
                             bool              aIsSecure,
                             bool              aIsHttpOnly);

    virtual ~nsCookie() {}

    
    inline const nsDependentCString Name()  const { return nsDependentCString(mName, mValue - 1); }
    inline const nsDependentCString Value() const { return nsDependentCString(mValue, mHost - 1); }
    inline const nsDependentCString Host()  const { return nsDependentCString(mHost, mPath - 1); }
    inline const nsDependentCString RawHost() const { return nsDependentCString(IsDomain() ? mHost + 1 : mHost, mPath - 1); }
    inline const nsDependentCString Path()  const { return nsDependentCString(mPath, mEnd); }
    inline PRInt64 Expiry()                 const { return mExpiry; }        
    inline PRInt64 LastAccessed()           const { return mLastAccessed; }  
    inline PRInt64 CreationTime()           const { return mCreationTime; }  
    inline bool IsSession()               const { return mIsSession; }
    inline bool IsDomain()                const { return *mHost == '.'; }
    inline bool IsSecure()                const { return mIsSecure; }
    inline bool IsHttpOnly()              const { return mIsHttpOnly; }

    
    inline void SetExpiry(PRInt64 aExpiry)        { mExpiry = aExpiry; }
    inline void SetLastAccessed(PRInt64 aTime)    { mLastAccessed = aTime; }
    inline void SetIsSession(bool aIsSession)   { mIsSession = (bool) aIsSession; }
    
    
    inline void SetCreationTime(PRInt64 aTime)    { mCreationTime = aTime; }

  protected:
    
    
    
    
    
    const char  *mName;
    const char  *mValue;
    const char  *mHost;
    const char  *mPath;
    const char  *mEnd;
    PRInt64      mExpiry;
    PRInt64      mLastAccessed;
    PRInt64      mCreationTime;
    bool mIsSession;
    bool mIsSecure;
    bool mIsHttpOnly;
};

#endif 
