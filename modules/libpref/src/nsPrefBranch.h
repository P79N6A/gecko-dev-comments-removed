







































#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIPrefLocalizedString.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIRelativeFilePref.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsWeakReference.h"
#include "nsClassHashtable.h"
#include "nsCRT.h"
#include "prbit.h"
#include "nsTraceRefcnt.h"

class nsPrefBranch;

class PrefCallback : public PLDHashEntryHdr {

  public:
    typedef PrefCallback* KeyType;
    typedef const PrefCallback* KeyTypePointer;

    static const PrefCallback* KeyToPointer(PrefCallback *aKey)
    {
      return aKey;
    }

    static PLDHashNumber HashKey(const PrefCallback *aKey)
    {
      PRUint32 strHash = nsCRT::HashCode(aKey->mDomain.BeginReading(),
                                         aKey->mDomain.Length());

      return PR_ROTATE_LEFT32(strHash, 4) ^
             NS_PTR_TO_UINT32(aKey->mCanonical);
    }


  public:
    
    PrefCallback(const char *aDomain, nsIObserver *aObserver,
                 nsPrefBranch *aBranch)
      : mDomain(aDomain),
        mBranch(aBranch),
        mWeakRef(nsnull),
        mStrongRef(aObserver)
    {
      MOZ_COUNT_CTOR(PrefCallback);
      nsCOMPtr<nsISupports> canonical = do_QueryInterface(aObserver);
      mCanonical = canonical;
    }

    
    PrefCallback(const char *aDomain,
                 nsISupportsWeakReference *aObserver,
                 nsPrefBranch *aBranch)
      : mDomain(aDomain),
        mBranch(aBranch),
        mWeakRef(do_GetWeakReference(aObserver)),
        mStrongRef(nsnull)
    {
      MOZ_COUNT_CTOR(PrefCallback);
      nsCOMPtr<nsISupports> canonical = do_QueryInterface(aObserver);
      mCanonical = canonical;
    }

    
    PrefCallback(const PrefCallback *&aCopy)
      : mDomain(aCopy->mDomain),
        mBranch(aCopy->mBranch),
        mWeakRef(aCopy->mWeakRef),
        mStrongRef(aCopy->mStrongRef),
        mCanonical(aCopy->mCanonical)
    {
      MOZ_COUNT_CTOR(PrefCallback);
    }

    ~PrefCallback()
    {
      MOZ_COUNT_DTOR(PrefCallback);
    }

    bool KeyEquals(const PrefCallback *aKey) const
    {
      
      
      
      
      
      
      
      
      
      
      
      
      

      if (IsExpired() || aKey->IsExpired())
        return this == aKey;

      if (mCanonical != aKey->mCanonical)
        return false;

      return mDomain.Equals(aKey->mDomain);
    }

    PrefCallback *GetKey() const
    {
      return const_cast<PrefCallback*>(this);
    }

    
    
    already_AddRefed<nsIObserver> GetObserver() const
    {
      if (!IsWeak()) {
        nsCOMPtr<nsIObserver> copy = mStrongRef;
        return copy.forget();
      }

      nsCOMPtr<nsIObserver> observer = do_QueryReferent(mWeakRef);
      return observer.forget();
    }

    const nsCString& GetDomain() const
    {
      return mDomain;
    }

    nsPrefBranch* GetPrefBranch() const
    {
      return mBranch;
    }

    
    bool IsExpired() const
    {
      if (!IsWeak())
        return false;

      nsCOMPtr<nsIObserver> observer(do_QueryReferent(mWeakRef));
      return !observer;
    }

    enum { ALLOW_MEMMOVE = true };

  private:
    nsCString             mDomain;
    nsPrefBranch         *mBranch;

    
    nsWeakPtr             mWeakRef;
    nsCOMPtr<nsIObserver> mStrongRef;

    
    nsISupports          *mCanonical;

    bool IsWeak() const
    {
      return !!mWeakRef;
    }
};

class nsPrefBranch : public nsIPrefBranchInternal,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPREFBRANCH
  NS_DECL_NSIPREFBRANCH2
  NS_DECL_NSIOBSERVER

  nsPrefBranch(const char *aPrefRoot, bool aDefaultBranch);
  virtual ~nsPrefBranch();

  PRInt32 GetRootLength() { return mPrefRootLength; }

  nsresult RemoveObserverFromMap(const char *aDomain, nsISupports *aObserver);

  static nsresult NotifyObserver(const char *newpref, void *data);

protected:
  nsPrefBranch()    
    { }

  nsresult   GetDefaultFromPropertiesFile(const char *aPrefName, PRUnichar **return_buf);
  void RemoveExpiredCallback(PrefCallback *aCallback);
  const char *getPrefName(const char *aPrefName);
  void       freeObserverList(void);

  friend PLDHashOperator
    FreeObserverFunc(PrefCallback *aKey,
                     nsAutoPtr<PrefCallback> &aCallback,
                     void *aArgs);

private:
  PRInt32               mPrefRootLength;
  nsCString             mPrefRoot;
  bool                  mIsDefault;

  bool                  mFreeingObserverList;
  nsClassHashtable<PrefCallback, PrefCallback> mObservers;
};


class nsPrefLocalizedString : public nsIPrefLocalizedString,
                              public nsISupportsString
{
public:
  nsPrefLocalizedString();
  virtual ~nsPrefLocalizedString();

  NS_DECL_ISUPPORTS
  NS_FORWARD_NSISUPPORTSSTRING(mUnicodeString->)
  NS_FORWARD_NSISUPPORTSPRIMITIVE(mUnicodeString->)

  nsresult Init();

private:
  NS_IMETHOD GetData(PRUnichar**);
  NS_IMETHOD SetData(const PRUnichar* aData);
  NS_IMETHOD SetDataWithLength(PRUint32 aLength, const PRUnichar *aData);

  nsCOMPtr<nsISupportsString> mUnicodeString;
};


class nsRelativeFilePref : public nsIRelativeFilePref
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRELATIVEFILEPREF
  
                nsRelativeFilePref();
  virtual       ~nsRelativeFilePref();
  
private:
  nsCOMPtr<nsILocalFile> mFile;
  nsCString mRelativeToKey;
};
