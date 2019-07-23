




































#ifndef nsTHashKeys_h__
#define nsTHashKeys_h__

#include "nsID.h"
#include "nsISupports.h"
#include "nsIHashable.h"
#include "nsCOMPtr.h"
#include "pldhash.h"
#include NEW_H

#include "nsStringGlue.h"
#include "nsCRTGlue.h"

#include <stdlib.h>
#include <string.h>



















NS_COM_GLUE PRUint32 HashString(const nsAString& aStr);
NS_COM_GLUE PRUint32 HashString(const nsACString& aStr);
NS_COM_GLUE PRUint32 HashString(const char* aKey);
NS_COM_GLUE PRUint32 HashString(const PRUnichar* aKey);






class nsStringHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsAString& KeyType;
  typedef const nsAString* KeyTypePointer;

  nsStringHashKey(KeyTypePointer aStr) : mStr(*aStr) { }
  nsStringHashKey(const nsStringHashKey& toCopy) : mStr(toCopy.mStr) { }
  ~nsStringHashKey() { }

  KeyType GetKey() const { return mStr; }
  KeyTypePointer GetKeyPointer() const { return &mStr; }
  PRBool KeyEquals(const KeyTypePointer aKey) const
  {
    return mStr.Equals(*aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(const KeyTypePointer aKey)
  {
    return HashString(*aKey);
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const nsString mStr;
};






class nsCStringHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsACString& KeyType;
  typedef const nsACString* KeyTypePointer;
  
  nsCStringHashKey(const nsACString* aStr) : mStr(*aStr) { }
  nsCStringHashKey(const nsCStringHashKey& toCopy) : mStr(toCopy.mStr) { }
  ~nsCStringHashKey() { }

  KeyType GetKey() const { return mStr; }
  KeyTypePointer GetKeyPointer() const { return &mStr; }

  PRBool KeyEquals(KeyTypePointer aKey) const { return mStr.Equals(*aKey); }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return HashString(*aKey);
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const nsCString mStr;
};






class nsUint32HashKey : public PLDHashEntryHdr
{
public:
  typedef const PRUint32& KeyType;
  typedef const PRUint32* KeyTypePointer;
  
  nsUint32HashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  nsUint32HashKey(const nsUint32HashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsUint32HashKey() { }

  KeyType GetKey() const { return mValue; }
  KeyTypePointer GetKeyPointer() const { return &mValue; }
  PRBool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return *aKey; }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const PRUint32 mValue;
};






class nsISupportsHashKey : public PLDHashEntryHdr
{
public:
  typedef nsISupports* KeyType;
  typedef const nsISupports* KeyTypePointer;

  nsISupportsHashKey(const nsISupports* key) :
    mSupports(NS_CONST_CAST(nsISupports*,key)) { }
  nsISupportsHashKey(const nsISupportsHashKey& toCopy) :
    mSupports(toCopy.mSupports) { }
  ~nsISupportsHashKey() { }

  KeyType GetKey() const { return mSupports; }
  KeyTypePointer GetKeyPointer() const { return mSupports; }
  
  PRBool KeyEquals(KeyTypePointer aKey) const { return aKey == mSupports; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_INT32(aKey) >>2;
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  nsCOMPtr<nsISupports> mSupports;
};






class nsVoidPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef const void* KeyType;
  typedef const void* KeyTypePointer;

  nsVoidPtrHashKey(const void* key) :
    mKey(key) { }
  nsVoidPtrHashKey(const nsVoidPtrHashKey& toCopy) :
    mKey(toCopy.mKey) { }
  ~nsVoidPtrHashKey() { }

  KeyType GetKey() const { return mKey; }
  KeyTypePointer GetKeyPointer() const { return mKey; }
  
  PRBool KeyEquals(KeyTypePointer aKey) const { return aKey == mKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_INT32(aKey) >>2;
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const void* mKey;
};









class nsClearingVoidPtrHashKey : public PLDHashEntryHdr
{
public:
  typedef const void* KeyType;
  typedef const void* KeyTypePointer;

  nsClearingVoidPtrHashKey(const void* key) :
    mKey(key) { }
  nsClearingVoidPtrHashKey(const nsClearingVoidPtrHashKey& toCopy) :
    mKey(toCopy.mKey) { }
  ~nsClearingVoidPtrHashKey() { mKey = NULL; }

  KeyType GetKey() const { return mKey; }
  KeyTypePointer GetKeyPointer() const { return mKey; }
  
  PRBool KeyEquals(KeyTypePointer aKey) const { return aKey == mKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return NS_PTR_TO_INT32(aKey) >>2;
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const void* mKey;
};






class nsIDHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsID& KeyType;
  typedef const nsID* KeyTypePointer;
  
  nsIDHashKey(const nsID* inID) : mID(*inID) { }
  nsIDHashKey(const nsIDHashKey& toCopy) : mID(toCopy.mID) { }
  ~nsIDHashKey() { }

  KeyType GetKey() const { return mID; }
  KeyTypePointer GetKeyPointer() const { return &mID; }

  PRBool KeyEquals(KeyTypePointer aKey) const { return aKey->Equals(mID); }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey);
  enum { ALLOW_MEMMOVE = PR_TRUE };

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
  const char* GetKeyPointer() const { return mKey; }
  PRBool KeyEquals(const char* aKey) const
  {
    return !strcmp(mKey, aKey);
  }

  static const char* KeyToPointer(const char* aKey) { return aKey; }
  static PLDHashNumber HashKey(const char* aKey) { return HashString(aKey); }
  enum { ALLOW_MEMMOVE = PR_TRUE };

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
  ~nsCharPtrHashKey() { if (mKey) free(NS_CONST_CAST(char *, mKey)); }

  const char* GetKey() const { return mKey; }
  const char* GetKeyPointer() const { return mKey; }
  PRBool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mKey, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return HashString(aKey); }

  enum { ALLOW_MEMMOVE = PR_TRUE };

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
  ~nsUnicharPtrHashKey() { if (mKey) NS_Free(NS_CONST_CAST(PRUnichar *, mKey)); }

  const PRUnichar* GetKey() const { return mKey; }
  const PRUnichar* GetKeyPointer() const { return mKey; }
  PRBool KeyEquals(KeyTypePointer aKey) const
  {
    return !NS_strcmp(mKey, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return HashString(aKey); }

  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const PRUnichar* mKey;
};




class nsHashableHashKey : public PLDHashEntryHdr
{
public:
    typedef nsIHashable* KeyType;
    typedef const nsIHashable* KeyTypePointer;

    nsHashableHashKey(const nsIHashable* aKey) :
        mKey(NS_CONST_CAST(nsIHashable*, aKey)) { }
    nsHashableHashKey(const nsHashableHashKey& toCopy) :
        mKey(toCopy.mKey) { }
    ~nsHashableHashKey() { }

    nsIHashable* GetKey() const { return mKey; }
    const nsIHashable* GetKeyPointer() const { return mKey; }

    PRBool KeyEquals(const nsIHashable* aKey) const {
        PRBool eq;
        if (NS_SUCCEEDED(mKey->Equals(NS_CONST_CAST(nsIHashable*,aKey), &eq))) {
            return eq;
        }
        return PR_FALSE;
    }

    static const nsIHashable* KeyToPointer(nsIHashable* aKey) { return aKey; }
    static PLDHashNumber HashKey(const nsIHashable* aKey) {
        PRUint32 code = 8888; 
#ifdef NS_DEBUG
        nsresult rv =
#endif
        NS_CONST_CAST(nsIHashable*,aKey)->GetHashCode(&code);
        NS_ASSERTION(NS_SUCCEEDED(rv), "GetHashCode should not throw!");
        return code;
    }
    
    enum { ALLOW_MEMMOVE = PR_TRUE };

private:
    nsCOMPtr<nsIHashable> mKey;
};

#endif 
