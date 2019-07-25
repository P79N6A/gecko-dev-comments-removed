






































#ifndef nsSHistory_h
#define nsSHistory_h


#include "nsCOMPtr.h"


#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsISHTransaction.h"
#include "nsIWebNavigation.h"
#include "nsIWeakReference.h"
#include "nsISimpleEnumerator.h"
#include "nsISHistoryListener.h"
#include "nsIHistoryEntry.h"
#include "nsIObserver.h"


#include "prclist.h"

class nsIDocShell;
class nsSHEnumerator;
class nsSHistoryObserver;
class nsSHistory: public PRCList,
                  public nsISHistory,
                  public nsISHistoryInternal,
                  public nsIWebNavigation
{
public:
  nsSHistory();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHISTORY
  NS_DECL_NSISHISTORYINTERNAL
  NS_DECL_NSIWEBNAVIGATION

  
  static nsresult Startup();
  static void Shutdown();
  static void UpdatePrefs();

  
  
  
  
  static PRUint32 GetMaxTotalViewers() { return sHistoryMaxTotalViewers; }

protected:
  virtual ~nsSHistory();
  friend class nsSHEnumerator;
  friend class nsSHistoryObserver;

   
   NS_IMETHOD GetEntryAtIndex(PRInt32 aIndex, bool aModifyIndex, nsISHEntry** aResult);
   NS_IMETHOD GetTransactionAtIndex(PRInt32 aIndex, nsISHTransaction ** aResult);
   nsresult CompareFrames(nsISHEntry * prevEntry, nsISHEntry * nextEntry, nsIDocShell * rootDocShell, long aLoadType, bool * aIsFrameFound);
   nsresult InitiateLoad(nsISHEntry * aFrameEntry, nsIDocShell * aFrameDS, long aLoadType);

   NS_IMETHOD LoadEntry(PRInt32 aIndex, long aLoadType, PRUint32 histCmd);

#ifdef DEBUG
   nsresult PrintHistory();
#endif

  
  
  void EvictOutOfRangeWindowContentViewers(PRInt32 aIndex);
  static void GloballyEvictContentViewers();
  static void GloballyEvictAllContentViewers();

  
  
  static PRUint32 CalcMaxTotalViewers();

  void RemoveDynEntries(PRInt32 aOldIndex, PRInt32 aNewIndex);

  nsresult LoadNextPossibleEntry(PRInt32 aNewIndex, long aLoadType, PRUint32 aHistCmd);
protected:
  
  
  
  bool RemoveDuplicate(PRInt32 aIndex, bool aKeepNext);

  nsCOMPtr<nsISHTransaction> mListRoot;
  PRInt32 mIndex;
  PRInt32 mLength;
  PRInt32 mRequestedIndex;
  
  nsWeakPtr mListener;
  
  nsIDocShell *  mRootDocShell;

  
  static PRInt32  sHistoryMaxTotalViewers;
};



class nsSHEnumerator : public nsISimpleEnumerator
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  nsSHEnumerator(nsSHistory *  aHistory);
  
protected:
  friend class nsSHistory;
  virtual ~nsSHEnumerator();
private:
  PRInt32     mIndex;
  nsSHistory *  mSHistory;  
};

#endif   
