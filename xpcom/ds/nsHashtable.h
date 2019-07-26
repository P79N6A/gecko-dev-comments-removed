



















#ifndef nsHashtable_h__
#define nsHashtable_h__

#include "pldhash.h"
#include "nscore.h"
#include "nsISupports.h"
#include "nsISupportsImpl.h"
#include "nsStringFwd.h"

class nsIObjectInputStream;
class nsIObjectOutputStream;

struct PRLock;

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
    virtual uint32_t HashCode(void) const = 0;
    virtual bool Equals(const nsHashKey *aKey) const = 0;
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
    kHashEnumerateStop      = false,
    kHashEnumerateNext      = true
};

typedef bool
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
    bool            mEnumerating;

  public:
    nsHashtable(uint32_t aSize = 16, bool aThreadSafe = false);
    virtual ~nsHashtable();

    int32_t Count(void) { return mHashtable.entryCount; }
    bool Exists(nsHashKey *aKey);
    void *Put(nsHashKey *aKey, void *aData);
    void *Get(nsHashKey *aKey);
    void *Remove(nsHashKey *aKey);
    nsHashtable *Clone();
    void Enumerate(nsHashtableEnumFunc aEnumFunc, void* aClosure = nullptr);
    void Reset();
    void Reset(nsHashtableEnumFunc destroyFunc, void* aClosure = nullptr);

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
                      uint32_t aSize = 16, bool threadSafe = false);
    ~nsObjectHashtable();

    nsHashtable *Clone();
    void Reset();
    bool RemoveAndDelete(nsHashKey *aKey);

  protected:
    static PLDHashOperator CopyElement(PLDHashTable* table,
                                       PLDHashEntryHdr* hdr,
                                       uint32_t i, void *arg);

    nsHashtableCloneElementFunc mCloneElementFun;
    void*                       mCloneElementClosure;
    nsHashtableEnumFunc         mDestroyElementFun;
    void*                       mDestroyElementClosure;
};



class nsPRUint32Key : public nsHashKey {
protected:
    uint32_t mKey;
public:
    nsPRUint32Key(uint32_t key) {
#ifdef DEBUG
        mKeyType = PRUint32Key;
#endif
        mKey = key;
    }

    uint32_t HashCode(void) const {
        return mKey;
    }

    bool Equals(const nsHashKey *aKey) const {
        return mKey == ((const nsPRUint32Key *) aKey)->mKey;
    }
    nsHashKey *Clone() const {
        return new nsPRUint32Key(mKey);
    }
    uint32_t GetValue() { return mKey; }
};


class nsCStringKey : public nsHashKey {
  public:

    
    enum Ownership {
        NEVER_OWN,  
        OWN_CLONE,  
        OWN         
    };

    nsCStringKey(const nsCStringKey& aStrKey);
    nsCStringKey(const char* str, int32_t strLen = -1, Ownership own = OWN_CLONE);
    nsCStringKey(const nsAFlatCString& str);
    nsCStringKey(const nsACString& str);
    ~nsCStringKey(void);

    uint32_t HashCode(void) const;
    bool Equals(const nsHashKey* aKey) const;
    nsHashKey* Clone() const;
    nsCStringKey(nsIObjectInputStream* aStream, nsresult *aResult);
    nsresult Write(nsIObjectOutputStream* aStream) const;

    
    
    const char* GetString() const { return mStr; }
    uint32_t GetStringLength() const { return mStrLen; }

  protected:
    char*       mStr;
    uint32_t    mStrLen;
    Ownership   mOwnership;
};



#endif
