




#ifndef nsTHashKeys_h__
#define nsTHashKeys_h__

#include "nsID.h"
#include "nsISupports.h"
#include "nsIHashable.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "pldhash.h"
#include NEW_H

#include "nsStringGlue.h"
#include "nsCRTGlue.h"
#include "nsUnicharUtils.h"

#include <stdlib.h>
#include <string.h>

#include "mozilla/HashFunctions.h"

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

  nsStringHashKey(KeyTypePointer aStr) : mStr(*aStr) { }
  nsStringHashKey(const nsStringHashKey& toCopy) : mStr(toCopy.mStr) { }
  ~nsStringHashKey() { }

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

  nsStringCaseInsensitiveHashKey(KeyTypePointer aStr) : mStr(*aStr) { } 
  nsStringCaseInsensitiveHashKey(const nsStringCaseInsensitiveHashKey& toCopy) : mStr(toCopy.mStr) { }
  ~nsStringCaseInsensitiveHashKey() { }

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

private:
  const nsString mStr;
};

#endif






class nsCStringHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsACString& KeyType;
  typedef const nsACString* KeyTypePointer;
  
  nsCStringHashKey(const nsACString* aStr) : mStr(*aStr) { }
  nsCStringHashKey(const nsCStringHashKey& toCopy) : mStr(toCopy.mStr) { }
  ~nsCStringHashKey() { }

  KeyType GetKey() const { return mStr; }

  bool KeyEquals(KeyTypePointer aKey) const { return mStr.Equals(*aKey); }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return mozilla::HashString(*aKey);
  }
  enum { ALLOW_MEMMOVE = true };

private:
  const nsCString mStr;
};






class nsUint32HashKey : public PLDHashEntryHdr
{
public:
  typedef const uint32_t& KeyType;
  typedef const uint32_t* KeyTypePointer;
  
  nsUint32HashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  nsUint32HashKey(const nsUint32HashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsUint32HashKey() { }

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
  
  nsUint64HashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  nsUint64HashKey(const nsUint64HashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsUint64HashKey() { }

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return PLDHashNumber(*aKey); }
  enum { ALLOW_MEMMOVE = true };

private:
  const uint64_t mValue;
};






class nsFloatHashKey : public PLDHashEntryHdr
{
public:
  typedef const float& KeyType;
  typedef const float* KeyTypePointer;

  nsFloatHashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  nsFloatHashKey(const nsFloatHashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsFloatHashKey() { }

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return *reinterpret_cast<const uint32_t*>(aKey); }
  enum { ALLOW_MEMMOVE = true };

private:
  const float mValue;
};






class nsISupportsHashKey : public PLDHashEntryHdr
{
public:
  typedef nsISupports* KeyType;
  typedef const nsISupports* KeyTypePointer;

  nsISupportsHashKey(const nsISupports* key) :
    mSupports(const_cast<nsISupports*>(key)) { }
  nsISupportsHashKey(const nsISupportsHashKey& toCopy) :
    mSupports(toCopy.mSupports) { }
  ~nsISupportsHashKey() { }

  KeyType GetKey() const { return mSupports; }
  
  bool KeyEquals(KeyTypePointer aKey) const { return aKey == mSupports; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_INT32(aKey) >>2;
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

  nsRefPtrHashKey(const T* key) :
    mKey(const_cast<T*>(key)) { }
  nsRefPtrHashKey(const nsRefPtrHashKey& toCopy) :
    mKey(toCopy.mKey) { }
  ~nsRefPtrHashKey() { }

  KeyType GetKey() const { return mKey; }
  
  bool KeyEquals(KeyTypePointer aKey) const { return aKey == mKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_INT32(aKey) >>2;
  }
  enum { ALLOW_MEMMOVE = true };

private:
  nsRefPtr<T> mKey;
};






template<class T>
class nsPtrHashKey : public PLDHashEntryHdr
{
 public:
  typedef T *KeyType;
  typedef const T *KeyTypePointer;

  nsPtrHashKey(const T *key) : mKey(const_cast<T*>(key)) {}
  nsPtrHashKey(const nsPtrHashKey<T> &toCopy) : mKey(toCopy.mKey) {}
  ~nsPtrHashKey() {}

  KeyType GetKey() const { return mKey; }

  bool KeyEquals(KeyTypePointer key) const { return key == mKey; }

  static KeyTypePointer KeyToPointer(KeyType key) { return key; }
  static PLDHashNumber HashKey(KeyTypePointer key)
  {
    return NS_PTR_TO_INT32(key) >> 2;
  }
  enum { ALLOW_MEMMOVE = true };

 protected:
  T *mKey;
};









template<class T>
class nsClearingPtrHashKey : public nsPtrHashKey<T>
{
 public:
  nsClearingPtrHashKey(const T *key) : nsPtrHashKey<T>(key) {}
  nsClearingPtrHashKey(const nsClearingPtrHashKey<T> &toCopy) :
    nsPtrHashKey<T>(toCopy) {}
  ~nsClearingPtrHashKey() { nsPtrHashKey<T>::mKey = nullptr; }
};

typedef nsPtrHashKey<const void> nsVoidPtrHashKey; 
typedef nsClearingPtrHashKey<const void> nsClearingVoidPtrHashKey;






class nsIDHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsID& KeyType;
  typedef const nsID* KeyTypePointer;
  
  nsIDHashKey(const nsID* inID) : mID(*inID) { }
  nsIDHashKey(const nsIDHashKey& toCopy) : mID(toCopy.mID) { }
  ~nsIDHashKey() { }

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

  nsDepCharHashKey(const char* aKey) { mKey = aKey; }
  nsDepCharHashKey(const nsDepCharHashKey& toCopy) { mKey = toCopy.mKey; }
  ~nsDepCharHashKey() { }

  const char* GetKey() const { return mKey; }
  bool KeyEquals(const char* aKey) const
  {
    return !strcmp(mKey, aKey);
  }

  static const char* KeyToPointer(const char* aKey) { return aKey; }
  static PLDHashNumber HashKey(const char* aKey) { return mozilla::HashString(aKey); }
  enum { ALLOW_MEMMOVE = true };

private:
  const char* mKey;
};






class nsCharPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  nsCharPtrHashKey(const char* aKey) : mKey(strdup(aKey)) { }
  nsCharPtrHashKey(const nsCharPtrHashKey& toCopy) : mKey(strdup(toCopy.mKey)) { }
  ~nsCharPtrHashKey() { if (mKey) free(const_cast<char *>(mKey)); }

  const char* GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mKey, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return mozilla::HashString(aKey); }

  enum { ALLOW_MEMMOVE = true };

private:
  const char* mKey;
};






class nsUnicharPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef const PRUnichar* KeyType;
  typedef const PRUnichar* KeyTypePointer;

  nsUnicharPtrHashKey(const PRUnichar* aKey) : mKey(NS_strdup(aKey)) { }
  nsUnicharPtrHashKey(const nsUnicharPtrHashKey& toCopy) : mKey(NS_strdup(toCopy.mKey)) { }
  ~nsUnicharPtrHashKey() { if (mKey) NS_Free(const_cast<PRUnichar *>(mKey)); }

  const PRUnichar* GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const
  {
    return !NS_strcmp(mKey, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return mozilla::HashString(aKey); }

  enum { ALLOW_MEMMOVE = true };

private:
  const PRUnichar* mKey;
};




class nsHashableHashKey : public PLDHashEntryHdr
{
public:
    typedef nsIHashable* KeyType;
    typedef const nsIHashable* KeyTypePointer;

    nsHashableHashKey(const nsIHashable* aKey) :
        mKey(const_cast<nsIHashable*>(aKey)) { }
    nsHashableHashKey(const nsHashableHashKey& toCopy) :
        mKey(toCopy.mKey) { }
    ~nsHashableHashKey() { }

    nsIHashable* GetKey() const { return mKey; }

    bool KeyEquals(const nsIHashable* aKey) const {
        bool eq;
        if (NS_SUCCEEDED(mKey->Equals(const_cast<nsIHashable*>(aKey), &eq))) {
            return eq;
        }
        return false;
    }

    static const nsIHashable* KeyToPointer(nsIHashable* aKey) { return aKey; }
    static PLDHashNumber HashKey(const nsIHashable* aKey) {
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

#endif 
