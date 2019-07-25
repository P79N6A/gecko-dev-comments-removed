




































#ifndef mozPersonalDictionary_h__
#define mozPersonalDictionary_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "mozIPersonalDictionary.h"
#include "nsIUnicodeEncoder.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsTHashtable.h"
#include "nsCRT.h"
#include "nsCycleCollectionParticipant.h"

#define MOZ_PERSONALDICTIONARY_CONTRACTID "@mozilla.org/spellchecker/personaldictionary;1"
#define MOZ_PERSONALDICTIONARY_CID         \
{ /* 7EF52EAF-B7E1-462B-87E2-5D1DBACA9048 */  \
0X7EF52EAF, 0XB7E1, 0X462B, \
  { 0X87, 0XE2, 0X5D, 0X1D, 0XBA, 0XCA, 0X90, 0X48 } }

class nsUniCharEntry : public PLDHashEntryHdr
{
public:
  
  typedef const PRUnichar* KeyType;
  typedef const PRUnichar* KeyTypePointer;

  nsUniCharEntry(const PRUnichar* aKey) : mKey(nsCRT::strdup(aKey)) {}
  nsUniCharEntry(const nsUniCharEntry& toCopy)
  { 
    NS_NOTREACHED("ALLOW_MEMMOVE is set, so copy ctor shouldn't be called");
  }

  ~nsUniCharEntry()
  { 
    if (mKey)
      nsCRT::free(mKey);
  }
 
  KeyType GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const { return !nsCRT::strcmp(mKey, aKey); }
  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }

  static PLDHashNumber HashKey(KeyTypePointer aKey) { return nsCRT::HashCode(aKey); }

  enum { ALLOW_MEMMOVE = true };

private:
  PRUnichar *mKey;
};


class mozPersonalDictionary : public mozIPersonalDictionary, 
                              public nsIObserver,
                              public nsSupportsWeakReference
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_MOZIPERSONALDICTIONARY
  NS_DECL_NSIOBSERVER
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(mozPersonalDictionary, mozIPersonalDictionary)

  mozPersonalDictionary();
  virtual ~mozPersonalDictionary();

  nsresult Init();

protected:
  bool           mDirty;       
  nsTHashtable<nsUniCharEntry> mDictionaryTable;
  nsTHashtable<nsUniCharEntry> mIgnoreTable;
  nsCOMPtr<nsIUnicodeEncoder>  mEncoder; 
};

#endif
