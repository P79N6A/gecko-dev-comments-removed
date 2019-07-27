




#ifndef nsPrefBranch_h
#define nsPrefBranch_h

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIPrefLocalizedString.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIRelativeFilePref.h"
#include "nsIFile.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWeakReference.h"
#include "nsClassHashtable.h"
#include "nsCRT.h"
#include "nsISupportsImpl.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {
class PreferenceServiceReporter;
} 

class nsPrefBranch;

class PrefCallback : public PLDHashEntryHdr {
  friend class mozilla::PreferenceServiceReporter;

  public:
    typedef PrefCallback* KeyType;
    typedef const PrefCallback* KeyTypePointer;

    static const PrefCallback* KeyToPointer(PrefCallback *aKey)
    {
      return aKey;
    }

    static PLDHashNumber HashKey(const PrefCallback *aKey)
    {
      uint32_t hash = mozilla::HashString(aKey->mDomain);
      return mozilla::AddToHash(hash, aKey->mCanonical);
    }


  public:
    
    PrefCallback(const char *aDomain, nsIObserver *aObserver,
                 nsPrefBranch *aBranch)
      : mDomain(aDomain),
        mBranch(aBranch),
        mWeakRef(nullptr),
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
        mStrongRef(nullptr)
    {
      MOZ_COUNT_CTOR(PrefCallback);
      nsCOMPtr<nsISupports> canonical = do_QueryInterface(aObserver);
      mCanonical = canonical;
    }

    
    explicit PrefCallback(const PrefCallback *&aCopy)
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
  friend class mozilla::PreferenceServiceReporter;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPREFBRANCH
  NS_DECL_NSIPREFBRANCH2
  NS_DECL_NSIOBSERVER

  nsPrefBranch(const char *aPrefRoot, bool aDefaultBranch);

  int32_t GetRootLength() { return mPrefRootLength; }

  nsresult RemoveObserverFromMap(const char *aDomain, nsISupports *aObserver);

  static void NotifyObserver(const char *newpref, void *data);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

protected:
  virtual ~nsPrefBranch();

  nsPrefBranch()    
    { }

  nsresult   GetDefaultFromPropertiesFile(const char *aPrefName, char16_t **return_buf);
  
  nsresult   SetCharPrefInternal(const char *aPrefName, const char *aValue);
  
  nsresult   CheckSanityOfStringLength(const char* aPrefName, const nsAString& aValue);
  nsresult   CheckSanityOfStringLength(const char* aPrefName, const char* aValue);
  nsresult   CheckSanityOfStringLength(const char* aPrefName, const uint32_t aLength);
  void RemoveExpiredCallback(PrefCallback *aCallback);
  const char *getPrefName(const char *aPrefName);
  void       freeObserverList(void);

  friend PLDHashOperator
    FreeObserverFunc(PrefCallback *aKey,
                     nsAutoPtr<PrefCallback> &aCallback,
                     void *aArgs);

private:
  int32_t               mPrefRootLength;
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

  NS_DECL_ISUPPORTS
  NS_FORWARD_NSISUPPORTSSTRING(mUnicodeString->)
  NS_FORWARD_NSISUPPORTSPRIMITIVE(mUnicodeString->)

  nsresult Init();

private:
  virtual ~nsPrefLocalizedString();

  NS_IMETHOD GetData(char16_t**);
  NS_IMETHOD SetData(const char16_t* aData);
  NS_IMETHOD SetDataWithLength(uint32_t aLength, const char16_t *aData);

  nsCOMPtr<nsISupportsString> mUnicodeString;
};


class nsRelativeFilePref : public nsIRelativeFilePref
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRELATIVEFILEPREF

  nsRelativeFilePref();

private:
  virtual ~nsRelativeFilePref();

  nsCOMPtr<nsIFile> mFile;
  nsCString mRelativeToKey;
};

#endif
