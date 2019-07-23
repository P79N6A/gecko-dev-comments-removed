











































#ifndef nsPtrHashKey_h_
#define nsPtrHashKey_h_

#include "pldhash.h"
#include "nscore.h"

template<class T>
class nsPtrHashKey : public PLDHashEntryHdr
{
 public:
  typedef const T *KeyType;
  typedef const T *KeyTypePointer;

  nsPtrHashKey(const T *key) : mKey(key) {}
  nsPtrHashKey(const nsPtrHashKey<T> &toCopy) : mKey(toCopy.mKey) {}
  ~nsPtrHashKey() {}

  KeyType GetKey() const { return mKey; }

  PRBool KeyEquals(KeyTypePointer key) const { return key == mKey; }

  static KeyTypePointer KeyToPointer(KeyType key) { return key; }
  static PLDHashNumber HashKey(KeyTypePointer key)
  {
    return NS_PTR_TO_INT32(key) >> 2;
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

 private:
  const T *mKey;
};

#endif  
