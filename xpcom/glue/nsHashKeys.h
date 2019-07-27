





#ifndef nsTHashKeys_h__
#define nsTHashKeys_h__

#include "nsID.h"
#include "nsISupports.h"
#include "nsIHashable.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "pldhash.h"
#include <new>

#include "nsStringGlue.h"
#include "nsCRTGlue.h"
#include "nsUnicharUtils.h"

#include <stdlib.h>
#include <string.h>

#include "mozilla/HashFunctions.h"
#include "mozilla/Move.h"

namespace mozilla {



inline uint32_t
HashString(const nsAString& aStr)
{
  return HashString(aStr.BeginReading(), aStr.Length());
}

inline uint32_t
HashString(const nsACString& aStr)
{
  return HashString(aStr.BeginReading(), aStr.Length());
}

} 





























class nsStringHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsAString& KeyType;
  typedef const nsAString* KeyTypePointer;

  explicit nsStringHashKey(KeyTypePointer aStr) : mStr(*aStr) {}
  nsStringHashKey(const nsStringHashKey& aToCopy) : mStr(aToCopy.mStr) {}
  ~nsStringHashKey() {}

  KeyType GetKey() const { return mStr; }
  bool KeyEquals(const KeyTypePointer aKey) const
  {
    return mStr.Equals(*aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(const KeyTypePointer aKey)
  {
    return mozilla::HashString(*aKey);
  }

#ifdef MOZILLA_INTERNAL_API
  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return GetKey().SizeOfExcludingThisMustBeUnshared(aMallocSizeOf);
  }
#endif

  enum { ALLOW_MEMMOVE = true };

private:
  const nsString mStr;
};

#ifdef MOZILLA_INTERNAL_API









class nsStringCaseInsensitiveHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsAString& KeyType;
  typedef const nsAString* KeyTypePointer;

  explicit nsStringCaseInsensitiveHashKey(KeyTypePointer aStr)
    : mStr(*aStr)
  {
    
  }
  nsStringCaseInsensitiveHashKey(const nsStringCaseInsensitiveHashKey& aToCopy)
    : mStr(aToCopy.mStr)
  {
  }
  ~nsStringCaseInsensitiveHashKey() {}

  KeyType GetKey() const { return mStr; }
  bool KeyEquals(const KeyTypePointer aKey) const
  {
    return mStr.Equals(*aKey, nsCaseInsensitiveStringComparator());
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(const KeyTypePointer aKey)
  {
    nsAutoString tmKey(*aKey);
    ToLowerCase(tmKey);
    return mozilla::HashString(tmKey);
  }
  enum { ALLOW_MEMMOVE = true };

  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return GetKey().SizeOfExcludingThisMustBeUnshared(aMallocSizeOf);
  }

private:
  const nsString mStr;
};

#endif






class nsCStringHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsACString& KeyType;
  typedef const nsACString* KeyTypePointer;

  explicit nsCStringHashKey(const nsACString* aStr) : mStr(*aStr) {}
  nsCStringHashKey(const nsCStringHashKey& aToCopy) : mStr(aToCopy.mStr) {}
  ~nsCStringHashKey() {}

  KeyType GetKey() const { return mStr; }
  bool KeyEquals(KeyTypePointer aKey) const { return mStr.Equals(*aKey); }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return mozilla::HashString(*aKey);
  }

#ifdef MOZILLA_INTERNAL_API
  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return GetKey().SizeOfExcludingThisMustBeUnshared(aMallocSizeOf);
  }
#endif

  enum { ALLOW_MEMMOVE = true };

private:
  const nsCString mStr;
};






class nsUint32HashKey : public PLDHashEntryHdr
{
public:
  typedef const uint32_t& KeyType;
  typedef const uint32_t* KeyTypePointer;

  explicit nsUint32HashKey(KeyTypePointer aKey) : mValue(*aKey) {}
  nsUint32HashKey(const nsUint32HashKey& aToCopy) : mValue(aToCopy.mValue) {}
  ~nsUint32HashKey() {}

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return *aKey; }
  enum { ALLOW_MEMMOVE = true };

private:
  const uint32_t mValue;
};






class nsUint64HashKey : public PLDHashEntryHdr
{
public:
  typedef const uint64_t& KeyType;
  typedef const uint64_t* KeyTypePointer;

  explicit nsUint64HashKey(KeyTypePointer aKey) : mValue(*aKey) {}
  nsUint64HashKey(const nsUint64HashKey& aToCopy) : mValue(aToCopy.mValue) {}
  ~nsUint64HashKey() {}

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return PLDHashNumber(*aKey);
  }
  enum { ALLOW_MEMMOVE = true };

private:
  const uint64_t mValue;
};






class nsFloatHashKey : public PLDHashEntryHdr
{
public:
  typedef const float& KeyType;
  typedef const float* KeyTypePointer;

  explicit nsFloatHashKey(KeyTypePointer aKey) : mValue(*aKey) {}
  nsFloatHashKey(const nsFloatHashKey& aToCopy) : mValue(aToCopy.mValue) {}
  ~nsFloatHashKey() {}

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return *reinterpret_cast<const uint32_t*>(aKey);
  }
  enum { ALLOW_MEMMOVE = true };

private:
  const float mValue;
};






class nsISupportsHashKey : public PLDHashEntryHdr
{
public:
  typedef nsISupports* KeyType;
  typedef const nsISupports* KeyTypePointer;

  explicit nsISupportsHashKey(const nsISupports* aKey)
    : mSupports(const_cast<nsISupports*>(aKey))
  {
  }
  nsISupportsHashKey(const nsISupportsHashKey& aToCopy)
    : mSupports(aToCopy.mSupports)
  {
  }
  ~nsISupportsHashKey() {}

  KeyType GetKey() const { return mSupports; }
  bool KeyEquals(KeyTypePointer aKey) const { return aKey == mSupports; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_UINT32(aKey) >> 2;
  }
  enum { ALLOW_MEMMOVE = true };

private:
  nsCOMPtr<nsISupports> mSupports;
};






template<class T>
class nsRefPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef T* KeyType;
  typedef const T* KeyTypePointer;

  explicit nsRefPtrHashKey(const T* aKey) : mKey(const_cast<T*>(aKey)) {}
  nsRefPtrHashKey(const nsRefPtrHashKey& aToCopy) : mKey(aToCopy.mKey) {}
  ~nsRefPtrHashKey() {}

  KeyType GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const { return aKey == mKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_UINT32(aKey) >> 2;
  }
  enum { ALLOW_MEMMOVE = true };

private:
  nsRefPtr<T> mKey;
};

template<class T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsRefPtrHashKey<T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteChild(aCallback, aField.GetKey(), aName, aFlags);
}






template<class T>
class nsPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef T* KeyType;
  typedef const T* KeyTypePointer;

  explicit nsPtrHashKey(const T* aKey) : mKey(const_cast<T*>(aKey)) {}
  nsPtrHashKey(const nsPtrHashKey<T>& aToCopy) : mKey(aToCopy.mKey) {}
  ~nsPtrHashKey() {}

  KeyType GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const { return aKey == mKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_UINT32(aKey) >> 2;
  }
  enum { ALLOW_MEMMOVE = true };

protected:
  T* mKey;
};









template<class T>
class nsClearingPtrHashKey : public nsPtrHashKey<T>
{
public:
  explicit nsClearingPtrHashKey(const T* aKey) : nsPtrHashKey<T>(aKey) {}
  nsClearingPtrHashKey(const nsClearingPtrHashKey<T>& aToCopy)
    : nsPtrHashKey<T>(aToCopy)
  {
  }
  ~nsClearingPtrHashKey() { nsPtrHashKey<T>::mKey = nullptr; }
};

typedef nsPtrHashKey<const void> nsVoidPtrHashKey;
typedef nsClearingPtrHashKey<const void> nsClearingVoidPtrHashKey;






template<class T>
class nsFuncPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef T& KeyType;
  typedef const T* KeyTypePointer;

  explicit nsFuncPtrHashKey(const T* aKey) : mKey(*const_cast<T*>(aKey)) {}
  nsFuncPtrHashKey(const nsFuncPtrHashKey<T>& aToCopy) : mKey(aToCopy.mKey) {}
  ~nsFuncPtrHashKey() {}

  KeyType GetKey() const { return const_cast<T&>(mKey); }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_UINT32(*aKey) >> 2;
  }
  enum { ALLOW_MEMMOVE = true };

protected:
  T mKey;
};






class nsIDHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsID& KeyType;
  typedef const nsID* KeyTypePointer;

  explicit nsIDHashKey(const nsID* aInID) : mID(*aInID) {}
  nsIDHashKey(const nsIDHashKey& aToCopy) : mID(aToCopy.mID) {}
  ~nsIDHashKey() {}

  KeyType GetKey() const { return mID; }
  bool KeyEquals(KeyTypePointer aKey) const { return aKey->Equals(mID); }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    return mozilla::HashBytes(aKey, sizeof(KeyType));
  }

  enum { ALLOW_MEMMOVE = true };

private:
  const nsID mID;
};











class nsDepCharHashKey : public PLDHashEntryHdr
{
public:
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  explicit nsDepCharHashKey(const char* aKey) : mKey(aKey) {}
  nsDepCharHashKey(const nsDepCharHashKey& aToCopy) : mKey(aToCopy.mKey) {}
  ~nsDepCharHashKey() {}

  const char* GetKey() const { return mKey; }
  bool KeyEquals(const char* aKey) const { return !strcmp(mKey, aKey); }

  static const char* KeyToPointer(const char* aKey) { return aKey; }
  static PLDHashNumber HashKey(const char* aKey)
  {
    return mozilla::HashString(aKey);
  }
  enum { ALLOW_MEMMOVE = true };

private:
  const char* mKey;
};






class nsCharPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  explicit nsCharPtrHashKey(const char* aKey) : mKey(strdup(aKey)) {}
  nsCharPtrHashKey(const nsCharPtrHashKey& aToCopy)
    : mKey(strdup(aToCopy.mKey))
  {
  }

  nsCharPtrHashKey(nsCharPtrHashKey&& aOther)
    : mKey(aOther.mKey)
  {
    aOther.mKey = nullptr;
  }

  ~nsCharPtrHashKey()
  {
    if (mKey) {
      free(const_cast<char*>(mKey));
    }
  }

  const char* GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const { return !strcmp(mKey, aKey); }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return mozilla::HashString(aKey);
  }

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(mKey);
  }

  enum { ALLOW_MEMMOVE = true };

private:
  const char* mKey;
};






class nsUnicharPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef const char16_t* KeyType;
  typedef const char16_t* KeyTypePointer;

  explicit nsUnicharPtrHashKey(const char16_t* aKey) : mKey(NS_strdup(aKey)) {}
  nsUnicharPtrHashKey(const nsUnicharPtrHashKey& aToCopy)
    : mKey(NS_strdup(aToCopy.mKey))
  {
  }

  nsUnicharPtrHashKey(nsUnicharPtrHashKey&& aOther)
    : mKey(aOther.mKey)
  {
    aOther.mKey = nullptr;
  }

  ~nsUnicharPtrHashKey()
  {
    if (mKey) {
      NS_Free(const_cast<char16_t*>(mKey));
    }
  }

  const char16_t* GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const { return !NS_strcmp(mKey, aKey); }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return mozilla::HashString(aKey);
  }

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(mKey);
  }

  enum { ALLOW_MEMMOVE = true };

private:
  const char16_t* mKey;
};




class nsHashableHashKey : public PLDHashEntryHdr
{
public:
  typedef nsIHashable* KeyType;
  typedef const nsIHashable* KeyTypePointer;

  explicit nsHashableHashKey(const nsIHashable* aKey)
    : mKey(const_cast<nsIHashable*>(aKey))
  {
  }
  nsHashableHashKey(const nsHashableHashKey& aToCopy) : mKey(aToCopy.mKey) {}
  ~nsHashableHashKey() {}

  nsIHashable* GetKey() const { return mKey; }

  bool KeyEquals(const nsIHashable* aKey) const
  {
    bool eq;
    if (NS_SUCCEEDED(mKey->Equals(const_cast<nsIHashable*>(aKey), &eq))) {
      return eq;
    }
    return false;
  }

  static const nsIHashable* KeyToPointer(nsIHashable* aKey) { return aKey; }
  static PLDHashNumber HashKey(const nsIHashable* aKey)
  {
    uint32_t code = 8888; 
#ifdef DEBUG
    nsresult rv =
#endif
      const_cast<nsIHashable*>(aKey)->GetHashCode(&code);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetHashCode should not throw!");
    return code;
  }

  enum { ALLOW_MEMMOVE = true };

private:
  nsCOMPtr<nsIHashable> mKey;
};





template<typename T>
class nsGenericHashKey : public PLDHashEntryHdr
{
public:
  typedef const T& KeyType;
  typedef const T* KeyTypePointer;

  explicit nsGenericHashKey(KeyTypePointer aKey) : mKey(*aKey) {}
  nsGenericHashKey(const nsGenericHashKey<T>& aOther) : mKey(aOther.mKey) {}

  KeyType GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return aKey->Hash(); }
  enum { ALLOW_MEMMOVE = true };

private:
  T mKey;
};

#endif 
