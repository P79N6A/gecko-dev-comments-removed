




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
             int64_t         aExpiry,
             int64_t         aLastAccessed,
             int64_t         aCreationTime,
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
     , mIsSession(aIsSession != false)
     , mIsSecure(aIsSecure != false)
     , mIsHttpOnly(aIsHttpOnly != false)
    {
    }

  public:
    
    
    static int64_t GenerateUniqueCreationTime(int64_t aCreationTime);

    
    
    static nsCookie * Create(const nsACString &aName,
                             const nsACString &aValue,
                             const nsACString &aHost,
                             const nsACString &aPath,
                             int64_t           aExpiry,
                             int64_t           aLastAccessed,
                             int64_t           aCreationTime,
                             bool              aIsSession,
                             bool              aIsSecure,
                             bool              aIsHttpOnly);

    virtual ~nsCookie() {}

    
    inline const nsDependentCString Name()  const { return nsDependentCString(mName, mValue - 1); }
    inline const nsDependentCString Value() const { return nsDependentCString(mValue, mHost - 1); }
    inline const nsDependentCString Host()  const { return nsDependentCString(mHost, mPath - 1); }
    inline const nsDependentCString RawHost() const { return nsDependentCString(IsDomain() ? mHost + 1 : mHost, mPath - 1); }
    inline const nsDependentCString Path()  const { return nsDependentCString(mPath, mEnd); }
    inline int64_t Expiry()                 const { return mExpiry; }        
    inline int64_t LastAccessed()           const { return mLastAccessed; }  
    inline int64_t CreationTime()           const { return mCreationTime; }  
    inline bool IsSession()               const { return mIsSession; }
    inline bool IsDomain()                const { return *mHost == '.'; }
    inline bool IsSecure()                const { return mIsSecure; }
    inline bool IsHttpOnly()              const { return mIsHttpOnly; }

    
    inline void SetExpiry(int64_t aExpiry)        { mExpiry = aExpiry; }
    inline void SetLastAccessed(int64_t aTime)    { mLastAccessed = aTime; }
    inline void SetIsSession(bool aIsSession)   { mIsSession = (bool) aIsSession; }
    
    
    inline void SetCreationTime(int64_t aTime)    { mCreationTime = aTime; }

  protected:
    
    
    
    
    
    const char  *mName;
    const char  *mValue;
    const char  *mHost;
    const char  *mPath;
    const char  *mEnd;
    int64_t      mExpiry;
    int64_t      mLastAccessed;
    int64_t      mCreationTime;
    bool mIsSession;
    bool mIsSecure;
    bool mIsHttpOnly;
};

#endif 
