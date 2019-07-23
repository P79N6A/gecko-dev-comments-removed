



































#ifndef nsUrlClassifierUtils_h_
#define nsUrlClassifierUtils_h_

#include "nsAutoPtr.h"
#include "nsIUrlClassifierUtils.h"
#include "nsTArray.h"
#include "nsDataHashtable.h"

class nsUrlClassifierUtils : public nsIUrlClassifierUtils
{
private:
  





  class Charmap
  {
  public:
    Charmap(PRUint32 b0, PRUint32 b1, PRUint32 b2, PRUint32 b3,
            PRUint32 b4, PRUint32 b5, PRUint32 b6, PRUint32 b7)
    {
      mMap[0] = b0; mMap[1] = b1; mMap[2] = b2; mMap[3] = b3;
      mMap[4] = b4; mMap[5] = b5; mMap[6] = b6; mMap[7] = b7;
    }

    


    PRBool Contains(unsigned char c) const
    {
      return mMap[c >> 5] & (1 << (c & 31));
    }

  private:
    
    PRUint32 mMap[8];
  };


public:
  nsUrlClassifierUtils();
  ~nsUrlClassifierUtils() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERUTILS

  nsresult Init();

  nsresult CanonicalizeHostname(const nsACString & hostname,
                                nsACString & _retval);
  nsresult CanonicalizePath(const nsACString & url, nsACString & _retval);

  
  
  
  
  
  
  PRBool SpecialEncode(const nsACString & url,
                       PRBool foldSlashes,
                       nsACString & _retval);

  void ParseIPAddress(const nsACString & host, nsACString & _retval);
  void CanonicalNum(const nsACString & num,
                    PRUint32 bytes,
                    PRBool allowOctal,
                    nsACString & _retval);

  
  
  static void UnUrlsafeBase64(nsACString & str);

  
  
  static nsresult DecodeClientKey(const nsACString & clientKey,
                                  nsACString & _retval);
private:
  
  nsUrlClassifierUtils(const nsUrlClassifierUtils&);

  
  PRBool ShouldURLEscape(const unsigned char c) const;

  void CleanupHostname(const nsACString & host, nsACString & _retval);

  nsAutoPtr<Charmap> mEscapeCharmap;
};




class nsUrlClassifierFragmentSet
{
public:
  nsUrlClassifierFragmentSet() : mFirst(nsnull), mLast(nsnull), mCapacity(16) {}

  PRBool Init(PRUint32 maxEntries) {
    mCapacity = maxEntries;
    if (!mEntryStorage.SetCapacity(mCapacity))
      return PR_FALSE;

    if (!mEntries.Init())
      return PR_FALSE;

    return PR_TRUE;
  }

  PRBool Put(const nsACString &fragment) {
    Entry *entry = nsnull;
    if (mEntries.Get(fragment, &entry)) {
      
      
      UnlinkEntry(entry);
    } else {
      if (mEntryStorage.Length() < mEntryStorage.Capacity()) {
        entry = mEntryStorage.AppendElement();
        if (!entry)
          return PR_FALSE;
      } else {
        
        entry = mLast;
        UnlinkEntry(entry);
        mEntries.Remove(entry->mFragment);
      }
      entry->mFragment = fragment;
      mEntries.Put(fragment, entry);
    }

    LinkEntry(entry);

    return PR_TRUE;
  }

  PRBool Has(const nsACString &fragment, PRBool update = PR_TRUE) {
    Entry *entry = nsnull;
    PRBool exists = mEntries.Get(fragment, &entry);
    
    if (update && exists && entry != mFirst) {
      UnlinkEntry(entry);
      LinkEntry(entry);
    }

    return exists;
  }

  void Clear() {
    mFirst = mLast = nsnull;
    mEntries.Clear();
    mEntryStorage.Clear();
    mEntryStorage.SetCapacity(mCapacity);
  }

private:
  
  
  class Entry {
  public:
    Entry() : mNext(nsnull), mPrev(nsnull) {};
    ~Entry() { }

    Entry *mNext;
    Entry *mPrev;
    nsCString mFragment;
  };

  void LinkEntry(Entry *entry)
  {
    
    entry->mPrev = nsnull;
    entry->mNext = mFirst;
    if (mFirst) {
      mFirst->mPrev = entry;
    }
    mFirst = entry;
    if (!mLast) {
      mLast = entry;
    }
  }

  void UnlinkEntry(Entry *entry)
  {
    if (entry->mPrev)
      entry->mPrev->mNext = entry->mNext;
    else
      mFirst = entry->mNext;

    if (entry->mNext)
      entry->mNext->mPrev = entry->mPrev;
    else
      mLast = entry->mPrev;

    entry->mPrev = entry->mNext = nsnull;
  }

  
  Entry *mFirst;
  
  Entry *mLast;

  
  PRUint32 mCapacity;

  
  nsTArray<Entry> mEntryStorage;

  
  nsDataHashtable<nsCStringHashKey, Entry*> mEntries;
};

#endif 
