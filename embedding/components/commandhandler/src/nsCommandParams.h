





































#ifndef nsCommandParams_h__
#define nsCommandParams_h__

#include "nsString.h"

#include "nsICommandParams.h"

#include "nsCOMPtr.h"
#include "nsHashtable.h"

#include "pldhash.h"



class nsCommandParams : public nsICommandParams
{
public:

                      nsCommandParams();
  virtual             ~nsCommandParams();
  
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMMANDPARAMS

  nsresult            Init();
  
protected:

  struct HashEntry : public PLDHashEntryHdr
  {
    nsCString      mEntryName;

    PRUint8       mEntryType;
    union {
    
      PRBool                  mBoolean;
      PRInt32                 mLong;
      double                  mDouble;
      nsString*               mString;
      nsCString*              mCString;
    } mData;

    nsCOMPtr<nsISupports>   mISupports;    
    
    HashEntry(PRUint8 inType, const char * inEntryName)
    : mEntryName(inEntryName)
    , mEntryType(inType)
    {
      memset(&mData, 0, sizeof(mData));
      Reset(mEntryType);
    }

    HashEntry(const HashEntry& inRHS)
    : mEntryType(inRHS.mEntryType)
    {
      Reset(mEntryType);
      switch (mEntryType)
      {
        case eBooleanType:      mData.mBoolean = inRHS.mData.mBoolean;  break;
        case eLongType:         mData.mLong    = inRHS.mData.mLong;     break;
        case eDoubleType:       mData.mDouble  = inRHS.mData.mDouble;   break;
        case eWStringType:
          NS_ASSERTION(inRHS.mData.mString, "Source entry has no string");
          mData.mString = new nsString(*inRHS.mData.mString);
          break;      
        case eStringType:
          NS_ASSERTION(inRHS.mData.mCString, "Source entry has no string");
          mData.mCString = new nsCString(*inRHS.mData.mCString);
          break;      
        case eISupportsType:
          mISupports = inRHS.mISupports.get();    
          break;
        default:
          NS_ASSERTION(0, "Unknown type");
      }
    }
    
    ~HashEntry()
    {
      if (mEntryType == eWStringType)
        delete mData.mString;
      else if (mEntryType == eStringType)
        delete mData.mCString;
    }
    
    void Reset(PRUint8 inNewType)
    {
      switch (mEntryType)
      {
        case eNoType:                                       break;
        case eBooleanType:      mData.mBoolean = PR_FALSE;  break;
        case eLongType:         mData.mLong = 0;            break;
        case eDoubleType:       mData.mDouble = 0.0;        break;
        case eWStringType:      delete mData.mString; mData.mString = nsnull;     break;
        case eISupportsType:    mISupports = nsnull;        break;    
        case eStringType:       delete mData.mCString; mData.mCString = nsnull;   break;
        default:
          NS_ASSERTION(0, "Unknown type");
      }
      
      mEntryType = inNewType;
    }
    
  };


  HashEntry*          GetNamedEntry(const char * name);
  HashEntry*          GetIndexedEntry(PRInt32 index);
  PRUint32            GetNumEntries();
  
  nsresult            GetOrMakeEntry(const char * name, PRUint8 entryType, HashEntry*& outEntry);
  
protected:

  static const void * PR_CALLBACK  HashGetKey(PLDHashTable *table, PLDHashEntryHdr *entry);

  static PLDHashNumber PR_CALLBACK HashKey(PLDHashTable *table, const void *key);

  static PRBool PR_CALLBACK        HashMatchEntry(PLDHashTable *table,
                                                  const PLDHashEntryHdr *entry, const void *key);
  
  static void PR_CALLBACK          HashMoveEntry(PLDHashTable *table, const PLDHashEntryHdr *from,
                                                 PLDHashEntryHdr *to);
  
  static void PR_CALLBACK          HashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry);
  
                                  
protected:
  
  enum {
    eNumEntriesUnknown      = -1
  };

  
  
  
  
  
  PLDHashTable    mValuesHash;
  
  
  PRInt32         mCurEntry;
  PRInt32         mNumEntries;      
    
  static PLDHashTableOps    sHashOps;
};


#endif
