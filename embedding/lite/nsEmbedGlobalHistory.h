





































#ifndef __nsEmbedGlobalHistory_h__
#define __nsEmbedGlobalHistory_h__

#include "nsIGlobalHistory.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsILocalFile.h"
#include "nsString.h"

class nsHashtable;
class nsHashKey;
class nsCStringKey;
class HistoryEntry;






#define NS_EMBEDGLOBALHISTORY_CID \
  { 0x2f977d51, 0x5485, 0x11d4, \
  { 0x87, 0xe2, 0x00, 0x10, 0xa4, 0xe7, 0x5e, 0xf2 } }

class nsEmbedGlobalHistory: public nsIGlobalHistory,
                            public nsIObserver,
                            public nsSupportsWeakReference
{
public:
                    nsEmbedGlobalHistory();
  virtual           ~nsEmbedGlobalHistory();
  
  NS_IMETHOD        Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGLOBALHISTORY
  NS_DECL_NSIOBSERVER

protected:
  enum { kFlushModeAppend, kFlushModeFullWrite };
  
  nsresult          LoadData();
  nsresult          FlushData(PRIntn mode = kFlushModeFullWrite);
  nsresult          ResetData();

  nsresult          GetHistoryFile();

  PRBool            EntryHasExpired(HistoryEntry *entry);
  static PRIntn PR_CALLBACK enumRemoveEntryIfExpired(nsHashKey *aKey, void *aData, void* closure);

protected:
  PRBool            mDataIsLoaded;
  PRInt32           mEntriesAddedSinceFlush;
  nsCOMPtr<nsILocalFile>  mHistoryFile;
  nsHashtable       *mURLTable;
  PRInt64           mExpirationInterval;
};

#endif
