



















































#ifndef nsHashtable_h__
#define nsHashtable_h__

#include "pldhash.h"
#include "prlock.h"
#include "nscore.h"
#include "nsString.h"
#include "nsISupportsBase.h"
#include "nsTraceRefcnt.h"

class nsIObjectInputStream;
class nsIObjectOutputStream;

class nsHashtable;
class nsStringKey;

class nsHashKey {
  protected:
    nsHashKey(void) {
#ifdef DEBUG
        mKeyType = UnknownKey;
#endif
        MOZ_COUNT_CTOR(nsHashKey);
    }


  public:
    
    

    virtual ~nsHashKey(void);
    virtual PRUint32 HashCode(void) const = 0;
    virtual PRBool Equals(const nsHashKey *aKey) const = 0;
    virtual nsHashKey *Clone() const = 0;
    virtual nsresult Write(nsIObjectOutputStream* aStream) const;

#ifdef DEBUG
  public:
    
    enum nsHashKeyType {
        UnknownKey,
        SupportsKey,
        PRUint32Key,
        VoidKey,
        IDKey,
        CStringKey,
        StringKey
    };
    nsHashKeyType GetKeyType() const { return mKeyType; }
  protected:
    nsHashKeyType mKeyType;
#endif
};




enum {
    kHashEnumerateStop      = PR_FALSE,
    kHashEnumerateNext      = PR_TRUE
};

typedef PRBool
(* nsHashtableEnumFunc)(nsHashKey *aKey, void *aData, void* aClosure);

typedef nsresult
(* nsHashtableReadEntryFunc)(nsIObjectInputStream *aStream, nsHashKey **aKey,
                             void **aData);


typedef void
(* nsHashtableFreeEntryFunc)(nsIObjectInputStream *aStream, nsHashKey *aKey,
                             void *aData);

typedef nsresult
(* nsHashtableWriteDataFunc)(nsIObjectOutputStream *aStream, void *aData);

class nsHashtable {
  protected:
    
    PRLock*         mLock;
    PLDHashTable    mHashtable;
    PRBool          mEnumerating;

  public:
    nsHashtable(PRUint32 aSize = 16, PRBool threadSafe = PR_FALSE);
    virtual ~nsHashtable();

    PRInt32 Count(void) { return mHashtable.entryCount; }
    PRBool Exists(nsHashKey *aKey);
    void *Put(nsHashKey *aKey, void *aData);
    void *Get(nsHashKey *aKey);
    void *Remove(nsHashKey *aKey);
    nsHashtable *Clone();
    void Enumerate(nsHashtableEnumFunc aEnumFunc, void* aClosure = NULL);
    void Reset();
    void Reset(nsHashtableEnumFunc destroyFunc, void* aClosure = NULL);

    nsHashtable(nsIObjectInputStream* aStream,
                nsHashtableReadEntryFunc aReadEntryFunc,
                nsHashtableFreeEntryFunc aFreeEntryFunc,
                nsresult *aRetVal);
    nsresult Write(nsIObjectOutputStream* aStream,
                   nsHashtableWriteDataFunc aWriteDataFunc) const;
};





typedef void* (* nsHashtableCloneElementFunc)(nsHashKey *aKey, void *aData, void* aClosure);

class nsObjectHashtable : public nsHashtable {
  public:
    nsObjectHashtable(nsHashtableCloneElementFunc cloneElementFun,
                      void* cloneElementClosure,
                      nsHashtableEnumFunc destroyElementFun,
                      void* destroyElementClosure,
                      PRUint32 aSize = 16, PRBool threadSafe = PR_FALSE);
    ~nsObjectHashtable();

    nsHashtable *Clone();
    void Reset();
    PRBool RemoveAndDelete(nsHashKey *aKey);

  protected:
    static PLDHashOperator CopyElement(PLDHashTable* table,
                                       PLDHashEntryHdr* hdr,
                                       PRUint32 i, void *arg);
    
    nsHashtableCloneElementFunc mCloneElementFun;
    void*                       mCloneElementClosure;
    nsHashtableEnumFunc         mDestroyElementFun;
    void*                       mDestroyElementClosure;
};




class nsISupports;

class nsSupportsHashtable
  : private nsHashtable
{
  public:
    nsSupportsHashtable(PRUint32 aSize = 16, PRBool threadSafe = PR_FALSE)
      : nsHashtable(aSize, threadSafe) {}
    ~nsSupportsHashtable();

    PRInt32 Count(void) {
        return nsHashtable::Count();
    }
    PRBool Exists(nsHashKey *aKey) {
        return nsHashtable::Exists (aKey);
    }
    PRBool Put(nsHashKey *aKey,
               nsISupports *aData,
               nsISupports **value = nsnull);
    nsISupports* Get(nsHashKey *aKey);
    PRBool Remove(nsHashKey *aKey, nsISupports **value = nsnull);
    nsHashtable *Clone();
    void Enumerate(nsHashtableEnumFunc aEnumFunc, void* aClosure = NULL) {
        nsHashtable::Enumerate(aEnumFunc, aClosure);
    }
    void Reset();

  private:
    static PRBool ReleaseElement(nsHashKey *, void *, void *);
    static PLDHashOperator EnumerateCopy(PLDHashTable*,
                                         PLDHashEntryHdr* hdr,
                                         PRUint32 i, void *arg);
};




#include "nsISupports.h"

class nsISupportsKey : public nsHashKey {
  protected:
    nsISupports* mKey;
    
  public:
    nsISupportsKey(const nsISupportsKey& aKey) : mKey(aKey.mKey) {
#ifdef DEBUG
        mKeyType = SupportsKey;
#endif
        NS_IF_ADDREF(mKey);
    }

    nsISupportsKey(nsISupports* key) {
#ifdef DEBUG
        mKeyType = SupportsKey;
#endif
        mKey = key;
        NS_IF_ADDREF(mKey);
    }
    
    ~nsISupportsKey(void) {
        NS_IF_RELEASE(mKey);
    }
    
    PRUint32 HashCode(void) const {
        return NS_PTR_TO_INT32(mKey);
    }

    PRBool Equals(const nsHashKey *aKey) const {
        NS_ASSERTION(aKey->GetKeyType() == SupportsKey, "mismatched key types");
        return (mKey == ((nsISupportsKey *) aKey)->mKey);
    }

    nsHashKey *Clone() const {
        return new nsISupportsKey(mKey);
    }

    nsISupportsKey(nsIObjectInputStream* aStream, nsresult *aResult);
    nsresult Write(nsIObjectOutputStream* aStream) const;
};


class nsPRUint32Key : public nsHashKey {
protected:
    PRUint32 mKey;
public:
    nsPRUint32Key(PRUint32 key) {
#ifdef DEBUG
        mKeyType = PRUint32Key;
#endif
        mKey = key;
    }

    PRUint32 HashCode(void) const {
        return mKey;
    }

    PRBool Equals(const nsHashKey *aKey) const {
        return mKey == ((const nsPRUint32Key *) aKey)->mKey;
    }
    nsHashKey *Clone() const {
        return new nsPRUint32Key(mKey);
    }
    PRUint32 GetValue() { return mKey; }
};




class nsVoidKey : public nsHashKey {
  protected:
    void* mKey;
    
  public:
    nsVoidKey(const nsVoidKey& aKey) : mKey(aKey.mKey) {
#ifdef DEBUG
        mKeyType = aKey.mKeyType;
#endif
    }

    nsVoidKey(void* key) {
#ifdef DEBUG
        mKeyType = VoidKey;
#endif
        mKey = key;
    }
    
    PRUint32 HashCode(void) const {
        return NS_PTR_TO_INT32(mKey);
    }

    PRBool Equals(const nsHashKey *aKey) const {
        NS_ASSERTION(aKey->GetKeyType() == VoidKey, "mismatched key types");
        return (mKey == ((const nsVoidKey *) aKey)->mKey);
    }

    nsHashKey *Clone() const {
        return new nsVoidKey(mKey);
    }

    void* GetValue() { return mKey; }
};

#include "nsString.h"


class nsCStringKey : public nsHashKey {
  public:

    
    enum Ownership {
        NEVER_OWN,  
        OWN_CLONE,  
        OWN         
    };

    nsCStringKey(const nsCStringKey& aStrKey);
    nsCStringKey(const char* str, PRInt32 strLen = -1, Ownership own = OWN_CLONE);
    nsCStringKey(const nsAFlatCString& str);
    nsCStringKey(const nsACString& str);
    ~nsCStringKey(void);

    PRUint32 HashCode(void) const;
    PRBool Equals(const nsHashKey* aKey) const;
    nsHashKey* Clone() const;
    nsCStringKey(nsIObjectInputStream* aStream, nsresult *aResult);
    nsresult Write(nsIObjectOutputStream* aStream) const;

    
    
    const char* GetString() const { return mStr; }
    PRUint32 GetStringLength() const { return mStrLen; }

  protected:
    char*       mStr;
    PRUint32    mStrLen;
    Ownership   mOwnership;
};


class nsStringKey : public nsHashKey {
  public:

    
    enum Ownership {
        NEVER_OWN,  
        OWN_CLONE,  
        OWN         
    };

    nsStringKey(const nsStringKey& aKey);
    nsStringKey(const PRUnichar* str, PRInt32 strLen = -1, Ownership own = OWN_CLONE);
    nsStringKey(const nsAFlatString& str);
    nsStringKey(const nsAString& str);
    ~nsStringKey(void);

    PRUint32 HashCode(void) const;
    PRBool Equals(const nsHashKey* aKey) const;
    nsHashKey* Clone() const;
    nsStringKey(nsIObjectInputStream* aStream, nsresult *aResult);
    nsresult Write(nsIObjectOutputStream* aStream) const;

    
    
    const PRUnichar* GetString() const { return mStr; }
    PRUint32 GetStringLength() const { return mStrLen; }

  protected:
    PRUnichar*  mStr;
    PRUint32    mStrLen;
    Ownership   mOwnership;
};



#endif
